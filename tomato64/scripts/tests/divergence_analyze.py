#!/usr/bin/env python3
"""Classify FreshTomato -> Tomato64 diffs and flag what needs a human look.

The Tomato64 rule is: never delete or edit an upstream line.  Code that must not
run on Tomato64 is wrapped in a guard (#ifndef TOMATO64 in C, a
/* TOMATO64-REMOVE-BEGIN */ marker in www files) and code that is new is added
inside its own guard.  So a well-formed divergence shows up in a unified diff as
*additions only*.

Anything that removes or rewrites an upstream line is therefore suspicious, with
a few known-benign exceptions:

  BRANDING    the line only changed FreshTomato -> Tomato64 (any case, incl. the
              freshtomato.org / tomato64.org URLs)
  WHITESPACE  the line was only re-indented / re-wrapped
  MOVED       the exact line still exists elsewhere in the Tomato64 file

Everything else is reported for review.
"""

import argparse
import difflib
import fnmatch
import os
import re
import sys

# ---------------------------------------------------------------- normalising

BRAND_RE = re.compile(
    r"fresh[ _-]?tomato|tomato[ _-]?64"      # full names, and the .org domains
    r"|\bFT\b|\bT64\b",                    # abbreviations used in help text
    re.IGNORECASE,
)


def norm_ws(s):
    return " ".join(s.split())


def norm_brand(s):
    return BRAND_RE.sub("@brand@", s)


def key(s):
    """Collapse the two differences we consider benign by policy."""
    return norm_brand(norm_ws(s)).lower()


# A line the Tomato64 port added purely to open/close a guard.  These are
# structure, not content, so they are removed before pairing old against new.
GUARD_RE = re.compile(
    r"""^(
          \#\s*(if|ifdef|ifndef|else|elif|endif)\b.*TOMATO64      # C preprocessor
        | /\*\s*TOMATO64[A-Z0-9_-]*-(BEGIN|END)\s*\*/             # /* TOMATO64-...-BEGIN */
        | <!--\s*TOMATO64[A-Z0-9_-]*-(BEGIN|END)\s*-->            # <!-- TOMATO64-... -->
        | \#\s*TOMATO64[A-Z0-9_-]*-(BEGIN|END)\s*$                # shell/make style
        | \#\s*endif\s*/\*.*TOMATO64.*\*/
        )$""",
    re.VERBOSE,
)


def is_guard(line):
    return bool(GUARD_RE.match(line.strip()))


# --------------------------------------------------------------- exceptions

class Exceptions:
    """Rules from exceptions.conf.

    ignore-file <path-glob>              skip the file entirely
    ignore-line <path-glob> <regex>      a changed line matching regex is not flagged
    ignore-hunk <path-glob> <regex>      any change block containing a matching
                                         line (either side) is not flagged
    <path-glob> is matched against "<package>/<file>", e.g. rc/Makefile or www/*.
    """

    def __init__(self, path=None):
        self.files, self.lines, self.hunks = [], [], []
        if not path or not os.path.exists(path):
            return
        with open(path, encoding="utf-8", errors="replace") as fh:
            for lineno, raw in enumerate(fh, 1):
                raw = raw.strip()
                if not raw or raw.startswith("#"):
                    continue
                parts = raw.split(None, 2)
                kind = parts[0]
                try:
                    if kind == "ignore-file" and len(parts) == 2:
                        self.files.append(parts[1])
                    elif kind == "ignore-line" and len(parts) == 3:
                        self.lines.append((parts[1], re.compile(parts[2])))
                    elif kind == "ignore-hunk" and len(parts) == 3:
                        self.hunks.append((parts[1], re.compile(parts[2])))
                    else:
                        raise ValueError("unrecognised rule")
                except (re.error, ValueError) as exc:
                    print(f"exceptions.conf:{lineno}: {exc}", file=sys.stderr)

    def file_ignored(self, path):
        return any(fnmatch.fnmatch(path, g) for g in self.files)

    def line_ignored(self, path, text):
        return any(fnmatch.fnmatch(path, g) and rx.search(text) for g, rx in self.lines)

    def block_ignored(self, path, texts):
        for g, rx in self.hunks:
            if fnmatch.fnmatch(path, g) and any(rx.search(t) for t in texts):
                return True
        return False


# ------------------------------------------------------------- patch parsing

class Block:
    """One contiguous run of '-' lines followed by '+' lines inside a hunk."""

    __slots__ = ("minus", "plus", "old_start", "new_start")

    def __init__(self, old_start, new_start):
        self.minus, self.plus = [], []
        self.old_start, self.new_start = old_start, new_start


HUNK_RE = re.compile(r"^@@ -(\d+)(?:,\d+)? \+(\d+)(?:,\d+)? @@")


def is_binary(path):
    """diff -a happily line-diffs a .gif; a NUL byte is the real test."""
    try:
        with open(path, "rb") as fh:
            return b"\0" in fh.read(8192)
    except OSError:
        return False


def parse_patch(path):
    """Yield Block objects.  Returns (blocks, binary_flag)."""
    blocks, binary = [], False
    old_no = new_no = 0
    cur = None
    with open(path, encoding="utf-8", errors="replace") as fh:
        for raw in fh:
            line = raw.rstrip("\n")
            if line.startswith("Binary files ") and line.endswith(" differ"):
                binary = True
                continue
            if line.startswith("---") or line.startswith("+++"):
                continue
            m = HUNK_RE.match(line)
            if m:
                if cur:
                    blocks.append(cur)
                    cur = None
                old_no, new_no = int(m.group(1)), int(m.group(2))
                continue
            if not line:
                # a context line that is genuinely empty
                if cur:
                    blocks.append(cur)
                    cur = None
                old_no += 1
                new_no += 1
                continue
            tag, body = line[0], line[1:]
            if tag == "-":
                if cur is None:
                    cur = Block(old_no, new_no)
                cur.minus.append(body)
                old_no += 1
            elif tag == "+":
                if cur is None:
                    cur = Block(old_no, new_no)
                cur.plus.append(body)
                new_no += 1
            elif tag == "\\":
                continue  # "\ No newline at end of file"
            else:
                if cur:
                    blocks.append(cur)
                    cur = None
                old_no += 1
                new_no += 1
    if cur:
        blocks.append(cur)
    return blocks, binary


# ------------------------------------------------------------- classification

REVIEW = "REVIEW"
INFO = "INFO"
OK = "OK"

SEVERITY = {
    "REMOVED": REVIEW,
    "MODIFIED": REVIEW,
    "BINARY": REVIEW,
    "MOVED": INFO,
    "WHITESPACE": INFO,
    "BRANDING": OK,
    "ADDED": OK,
}


class Finding:
    __slots__ = ("kind", "path", "old_no", "new_no", "old", "new")

    def __init__(self, kind, path, old_no, new_no, old=None, new=None):
        self.kind, self.path = kind, path
        self.old_no, self.new_no = old_no, new_no
        self.old, self.new = old, new

    @property
    def severity(self):
        return SEVERITY[self.kind]


def classify_pair(old, new):
    if norm_ws(old) == norm_ws(new):
        return "WHITESPACE"
    if key(old) == key(new):
        return "BRANDING"
    return "MODIFIED"


def analyse(patch_path, rel, ft_file, t64_file, exc):
    blocks, binary = parse_patch(patch_path)
    binary = binary or is_binary(ft_file) or is_binary(t64_file)
    findings, counts = [], {}

    def add(kind, old_no, new_no, old=None, new=None):
        counts[kind] = counts.get(kind, 0) + 1
        if SEVERITY[kind] != OK or kind == "BINARY":
            findings.append(Finding(kind, rel, old_no, new_no, old, new))

    if binary:
        add("BINARY", 0, 0)
        return findings, counts

    # every line still present in the Tomato64 file, for the MOVED test
    survivors = set()
    if os.path.exists(t64_file):
        with open(t64_file, encoding="utf-8", errors="replace") as fh:
            survivors = {ln.strip() for ln in fh if ln.strip()}
    # same, after branding/whitespace folding: diff never lines a rebranded
    # line up with its original, so it can land next to an unrelated addition
    survivor_keys = {key(ln) for ln in survivors}

    def relocated(old):
        if old.strip() in survivors:
            return "MOVED"
        if key(old) in survivor_keys:
            return "BRANDING"
        return None

    for blk in blocks:
        texts = blk.minus + blk.plus
        if exc.block_ignored(rel, texts):
            continue

        # guard directives are structural additions and blank lines are churn;
        # drop both before pairing so real content lines up with real content
        plus = [(i, t) for i, t in enumerate(blk.plus)
                if t.strip() and not is_guard(t)]
        counts["ADDED"] = counts.get("ADDED", 0) + (len(blk.plus) - len(plus))
        minus = [(i, t) for i, t in enumerate(blk.minus) if t.strip()]

        if not minus:
            counts["ADDED"] = counts.get("ADDED", 0) + len(plus)
            continue

        minus_keys = [key(t) for _, t in minus]
        plus_keys = [key(t) for _, t in plus]
        sm = difflib.SequenceMatcher(None, minus_keys, plus_keys, autojunk=False)

        paired = set()

        def pair(mi, pj):
            paired.add(mi)
            moff, old = minus[mi]
            poff, new = plus[pj]
            if exc.line_ignored(rel, old) or exc.line_ignored(rel, new):
                return
            kind = classify_pair(old, new)
            if kind == "MODIFIED":
                kind = relocated(old) or kind
            add(kind, blk.old_start + moff, blk.new_start + poff, old, new)

        for tag, i1, i2, j1, j2 in sm.get_opcodes():
            if tag in ("equal", "replace"):
                n = min(i2 - i1, j2 - j1)
                for off in range(n):
                    pair(i1 + off, j1 + off)
                counts["ADDED"] = counts.get("ADDED", 0) + max(0, (j2 - j1) - n)
            elif tag == "insert":
                counts["ADDED"] = counts.get("ADDED", 0) + (j2 - j1)

        for mi, (moff, old) in enumerate(minus):
            if mi in paired or exc.line_ignored(rel, old):
                continue
            kind = relocated(old) or "REMOVED"
            add(kind, blk.old_start + moff, blk.new_start, old, None)

    return findings, counts


# ------------------------------------------------------- www marker sanity

MARKER_RE = re.compile(r"TOMATO64([A-Z0-9_-]*?)-(BEGIN|END)\b")


def check_markers(t64_file):
    """Unbalanced web guards silently mangle a page at install time."""
    opens = {}
    problems = []
    try:
        with open(t64_file, encoding="utf-8", errors="replace") as fh:
            for lineno, raw in enumerate(fh, 1):
                for m in MARKER_RE.finditer(raw):
                    name = "TOMATO64" + m.group(1)
                    if m.group(2) == "BEGIN":
                        if name in opens:
                            problems.append(
                                f"line {lineno}: {name}-BEGIN reopened "
                                f"(previous at line {opens[name]})")
                        opens[name] = lineno
                    else:
                        if name not in opens:
                            problems.append(f"line {lineno}: {name}-END without BEGIN")
                        else:
                            del opens[name]
    except OSError:
        return problems
    for name, lineno in sorted(opens.items(), key=lambda kv: kv[1]):
        problems.append(f"line {lineno}: {name}-BEGIN never closed")
    return problems


# --------------------------------------------------------------------- main

def read_structure(path):
    pkgs, ft_only_files, ft_only_dirs = [], [], []
    if not os.path.exists(path):
        return pkgs, ft_only_files, ft_only_dirs
    with open(path, encoding="utf-8") as fh:
        for raw in fh:
            f = raw.rstrip("\n").split("\t")
            if f[0] == "PKG":
                pkgs.append((f[1], f[2], f[3], int(f[4]), int(f[5])))
            elif f[0] == "FT_ONLY_FILE":
                ft_only_files.append((f[1], f[2]))
            elif f[0] == "FT_ONLY_DIR":
                ft_only_dirs.append((f[1], f[2]))
    return pkgs, ft_only_files, ft_only_dirs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out-dir", required=True)
    ap.add_argument("--ft-root", required=True)
    ap.add_argument("--t64-root", required=True)
    ap.add_argument("--structure", required=True)
    ap.add_argument("--exceptions")
    ap.add_argument("--packages")
    ap.add_argument("--max-width", type=int, default=160,
                    help="truncate quoted lines (0 = never)")
    ap.add_argument("--verbose", action="store_true")
    args = ap.parse_args()

    exc = Exceptions(args.exceptions)
    pkgs, ft_only_files, ft_only_dirs = read_structure(args.structure)
    wanted = set(args.packages.split(",")) if args.packages else None

    review, info, skipped = [], [], []
    totals, per_file = {}, {}
    marker_problems = []

    for pkg, ftdir, t64dir, _n, _d in pkgs:
        if wanted and pkg not in wanted:
            continue
        pdir = os.path.join(args.out_dir, pkg)
        if not os.path.isdir(pdir):
            continue
        for name in sorted(os.listdir(pdir)):
            if not name.endswith(".patch"):
                continue
            src = name[:-len(".patch")]
            rel = f"{pkg}/{src}"
            t64_file = os.path.join(t64dir, src)

            if exc.file_ignored(rel):
                skipped.append(rel)
                continue

            findings, counts = analyse(os.path.join(pdir, name), rel,
                                       os.path.join(ftdir, src), t64_file, exc)
            per_file[rel] = counts
            for k, v in counts.items():
                totals[k] = totals.get(k, 0) + v
            for f in findings:
                (review if f.severity == REVIEW else info).append(f)

            if src.endswith((".asp", ".js", ".jsx", ".css", ".svg", ".html")):
                probs = check_markers(t64_file)
                if probs:
                    marker_problems.append((rel, probs))

    w = 78
    out = print

    out("=" * w)
    out(" FreshTomato -> Tomato64 divergence report")
    out("=" * w)
    out(f" upstream : {args.ft_root}")
    out(f" port     : {args.t64_root}")
    out("")
    for pkg, _ftdir, _t64dir, n, d in pkgs:
        if wanted and pkg not in wanted:
            continue
        out(f"   {pkg:<12} {d:>4} of {n:>4} shared files differ")
    out("")

    def clip(text):
        limit = args.max_width
        if limit and len(text) > limit:
            return text[:limit] + f"   ... (+{len(text) - limit} chars)"
        return text

    def show(f):
        if f.kind == "BINARY":
            out(f"  {f.path}")
            out("      binary files differ (nothing to diff by line)")
            return
        out(f"  {f.path}   [{f.kind}]")
        if f.old is not None:
            out(f"      FT  :{f.old_no:<6} -{clip(f.old)}")
        if f.new is not None:
            out(f"      T64 :{f.new_no:<6} +{clip(f.new)}")
        else:
            out(f"      T64 :{f.new_no:<6}  (line is gone -- not wrapped in a guard)")

    out("-" * w)
    out(f" NEEDS REVIEW -- upstream lines deleted or rewritten  ({len(review)})")
    out("-" * w)
    if review:
        cur = None
        for f in sorted(review, key=lambda x: (x.path, x.old_no)):
            if f.path != cur:
                out("")
                cur = f.path
            show(f)
    else:
        out("  none -- every difference is an addition, a guard, or branding.")
    out("")

    if ft_only_files or ft_only_dirs:
        out("-" * w)
        out(" UPSTREAM-ONLY -- present in FreshTomato, absent from Tomato64")
        out("-" * w)
        for pkg, f in ft_only_files:
            if wanted and pkg not in wanted:
                continue
            out(f"  {pkg}/{f}")
        for pkg, d in ft_only_dirs:
            if wanted and pkg not in wanted:
                continue
            out(f"  {pkg}/{d}/   (directory -- not compared)")
        out("")

    if marker_problems:
        out("-" * w)
        out(" WEB GUARD MARKERS -- unbalanced BEGIN/END in the Tomato64 file")
        out("-" * w)
        for rel, probs in marker_problems:
            out(f"  {rel}")
            for p in probs:
                out(f"      {p}")
        out("")

    if args.verbose and info:
        out("-" * w)
        out(f" INFO -- benign by policy, listed for completeness  ({len(info)})")
        out("-" * w)
        cur = None
        for f in sorted(info, key=lambda x: (x.path, x.old_no)):
            if f.path != cur:
                out("")
                cur = f.path
            show(f)
        out("")

    if skipped:
        out("-" * w)
        out(" SKIPPED by exceptions.conf")
        out("-" * w)
        for rel in skipped:
            out(f"  {rel}")
        out("")

    out("-" * w)
    out(" TOTALS")
    out("-" * w)
    for kind in ("REMOVED", "MODIFIED", "BINARY", "MOVED", "WHITESPACE",
                 "BRANDING", "ADDED"):
        if kind in totals:
            out(f"   {kind:<12} {totals[kind]:>6}   ({SEVERITY[kind]})")
    if not args.verbose and info:
        out(f"\n   {len(info)} INFO finding(s) hidden -- re-run with -v to see them")
    out("")

    return 1 if review or marker_problems else 0


if __name__ == "__main__":
    sys.exit(main())
