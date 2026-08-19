# divergence-check

Compares the sources Tomato64 shares with FreshTomato and flags differences that
may be unintentional.

```
./divergence-check.sh              # all packages
./divergence-check.sh rc www       # only these
./divergence-check.sh -v rc        # include INFO findings
./divergence-check.sh -w 0         # don't truncate long quoted lines
```

Paths can be overridden with `FT_ROOT`, `T64_ROOT`, `OUT_DIR`, `EXCEPTIONS`.
Defaults:

| var        | default                                                    |
|------------|------------------------------------------------------------|
| `FT_ROOT`  | `~/repos/freshtomato-arm/release/src-rt-6.x.4708/router`   |
| `T64_ROOT` | `~/tomato64/tomato64/package`                              |
| `OUT_DIR`  | `~/patch`                                                  |

Packages compared are listed in the `PACKAGES` array at the top of the script
(`./divergence-check.sh -l` lists them).  Add a row to compare another one.

## What it does

1. Computes each package's file list as the **intersection** of the two trees,
   so the list can't go stale.  Tomato64-only files are ignored; FreshTomato-only
   files and directories are reported, since those are usually upstream additions
   that have not been ported.
2. Writes `$OUT_DIR/<package>/<file>.patch` and deletes the empty ones.
3. Classifies every changed line and writes `$OUT_DIR/REPORT.txt`.

Exit status is 1 when anything needs review, 0 when clean.

## How lines are classified

The Tomato64 rule is that upstream lines are never deleted or edited — code that
must not run is wrapped in a guard (`#ifndef TOMATO64`, or a
`/* TOMATO64-REMOVE-BEGIN */` marker in `www`) and new code goes inside its own
guard.  A correct divergence therefore shows up in a unified diff as **additions
only**, which is what makes the check work.

| verdict      | meaning                                                        |
|--------------|----------------------------------------------------------------|
| `REMOVED`    | an upstream line is simply gone — **review**                   |
| `MODIFIED`   | an upstream line was rewritten, and not just for branding — **review** |
| `BINARY`     | the two files differ but can't be diffed by line — **review**  |
| `MOVED`      | the line is gone from this spot but still exists in the file    |
| `WHITESPACE` | re-indented only                                               |
| `BRANDING`   | only FreshTomato → Tomato64 changed (any case, incl. `FT`/`T64` and the `.org` URLs) |
| `ADDED`      | a new line, or a guard directive                               |

`www` files also get a marker sanity check: an unbalanced `TOMATO64*-BEGIN` /
`-END` pair silently mangles the page when `Makefile` strips it at install time.

## Exceptions

Anything confirmed intentional goes in `exceptions.conf` — see the comments in
that file for the three rule types (`ignore-file`, `ignore-line`, `ignore-hunk`).
