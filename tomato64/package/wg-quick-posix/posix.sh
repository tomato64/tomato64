#!/bin/sh

# -*- mode: sh; sh-indentation: 2; sh-basic-offset: 2; indent-tabs-mode: nil; fill-column: 100; coding: utf-8-unix; -*-
#
# SPDX-License-Identifier: GPL-2.0
#
# Copyright (C) 2015-2020 Jason A. Donenfeld <Jason at zx2c4.com>. All Rights Reserved.
# Austere posix/embedded variant derived by Rowan Thorpe <rowan at rowanthorpe.com>, 2021.
#
# original patch : https://lists.zx2c4.com/pipermail/wireguard/2021-June/006924.html
#
# Sanity checked with:
#  + shellcheck --check-sourced --external-sources --enable=all --shell=sh posix.sh
#  + shfmt -d -ln posix -i 2 -ci posix.sh
#
# TODO:
#  * Create local-vars drop-in functionality (without exploding complexity) to
#    ensure recursion doesn't shadow vars.

set -e

## setup needed before function-definitions

trap - EXIT
trap 'exit 1' HUP INT QUIT TERM

# primitive exit-trap stack to keep things manageable
exit_trap() {
  case "${1}" in
    push) EXIT_TRAP="${2}${EXIT_TRAP:+${NL}${EXIT_TRAP}}" ;;
    pop) EXIT_TRAP="$(printf '%s\n' "${EXIT_TRAP}" | tail -n +2)" ;;
    *) exit 1 ;;
  esac
  #shellcheck disable=SC2064
  trap "${EXIT_TRAP:--}" EXIT
}

# freshtomato doesn't have mktemp so we need to monkeypatch
mktemp() {
  FILENAME=$(cat /dev/urandom | tr -cd 'a-f0-9' | head -c 10)
  echo "" > /tmp/var/tmp/$FILENAME
  echo "/tmp/var/tmp/$FILENAME"
}

# freshtomato doesn't have readlink so we need to monkeypatch
readlink() {
  [ "${1:-}" ] || return 1
  max_symlinks=40
  CDPATH='' # to avoid changing to an unexpected directory

  target=$1
  [ -e "${target%/}" ] || target=${1%"${1##*[!/]}"} # trim trailing slashes
  [ -d "${target:-/}" ] && target="$target/"

  cd -P . 2>/dev/null || return 1
  while [ "$max_symlinks" -ge 0 ] && max_symlinks=$((max_symlinks - 1)); do
    if [ ! "$target" = "${target%/*}" ]; then
      case $target in
        /*) cd -P "${target%/*}/" 2>/dev/null || break ;;
        *) cd -P "./${target%/*}" 2>/dev/null || break ;;
      esac
      target=${target##*/}
    fi

    if [ ! -L "$target" ]; then
      target="${PWD%/}${target:+/}${target}"
      printf '%s\n' "${target:-/}"
      return 0
    fi

    # `ls -dl` format: "%s %u %s %s %u %s %s -> %s\n",
    #   <file mode>, <number of links>, <owner name>, <group name>,
    #   <size>, <date and time>, <pathname of link>, <contents of link>
    # https://pubs.opengroup.org/onlinepubs/9699919799/utilities/ls.html
    link=$(ls -dl -- "$target" 2>/dev/null) || break
    target=${link#*" $target -> "}
  done
  return 1
}

# embedded systems without char-classes in "tr" need monkeypatching
if ! [ "$(printf 'aBcD' | tr '[:upper:]' '[:lower:]')" = 'abcd' ]; then
  REAL_TR="$(type -p tr 2>/dev/null)"
  tr() {
    args=''
    while [ "${#}" -ne 0 ]; do
      case "${1}" in
        '[:upper:]')
          args="${args:+${args} }$(entity_save '[A-Z]')"
          ;;
        '[:lower:]')
          args="${args:+${args} }$(entity_save '[a-z]')"
          ;;
        *)
          args="${args:+${args} }$(entity_save "${1}")"
          ;;
      esac
      shift
    done
    eval "${REAL_TR} ${args}"
    unset args
  }
fi

# POSIX shells _may_ not have "type -p" so we need this drop-in
#shellcheck disable=SC2039
if [ -n "$(type -p cat 2>/dev/null || :)" ]; then
  type_p() {
    type -p "${@}"
  }
else
  type_p() {
    ret=0
    for arg; do
      found=0
      for path in $(printf %s "${PATH-}" | tr ':' ' '); do
        if [ -x "${path}/${arg}" ]; then
          found=1
          break
        fi
      done
      if [ "${found}" -eq 1 ]; then
        printf '%s/%s' "${path}" "${arg}"
      else
        ret=1
      fi
    done
    unset arg found path
    if [ "${ret}" -eq 0 ]; then
      unset ret
      return 0
    else
      unset ret
      return 1
    fi
  }
fi

# embedded systems without "stat" need this drop-in
if type -p stat >/dev/null 2>&1; then
  stat_octal() {
    stat -c '%04a' "${@}"
  }
else
  stat_octal() {
    #shellcheck disable=SC2012 disable=SC2034
    ls -l "${@}" |
      sed -ne '
        s/^[-dsbclp]\([-r]\)\([-w]\)\([-xsStT]\)\([-r]\)\([-w]\)\([-xsStT]\)\([-r]\)\([-w]\)\([-xsStT]\).*$/\1 \2 \3 \4 \5 \6 \7 \8 \9/g
        t P
        b
        : P
        p
      ' |
      while read -r ur uw ux gr gw gx or ow ox; do
        out=''
        spc_sum=0
        for ctg in u g o; do
          sum=0
          for perm in r w x; do
            var="${ctg}${perm}"
            eval "val=\"\${${var}}\""
            #shellcheck disable=SC2154
            case "${val}" in
              r) exp=2 ;;
              w) exp=1 ;;
              s | t | x) exp=0 ;;
              - | S | T) exp=-1 ;;
              *) exit 1 ;;
            esac
            case "${val}" in
              - | w | r | x)
                spc_exp=-1
                ;;
              S | s)
                case "${var}" in
                  u*) spc_exp=2 ;;
                  g*) spc_exp=1 ;;
                  *) exit 1 ;;
                esac
                ;;
              T | t)
                case "${var}" in
                  o*) spc_exp=0 ;;
                  *) exit 1 ;;
                esac
                ;;
              *)
                exit 1
                ;;
            esac
            [ "${exp}" -lt 0 ] ||
              sum=$((sum + $((1 << exp))))
            [ "${spc_exp}" -lt 0 ] ||
              spc_sum=$((spc_sum + $((1 << spc_exp))))
          done
          out="${out}$(printf %o "${sum}")"
        done
        printf '%o%s\n' "${spc_sum}" "${out}"
      done
    unset ur uw ux gr gw gx or ow ox ctg spc_sum out perm sum var val exp spc_exp
  }
fi

##

e_body_save() { sed -e "s/'/'\\\\''/g"; }

e_head_save() { sed -e "1s/^/'/"; }

e_tail_save() { sed -e "\$s/\$/'/"; }

e_save() { e_body_save | e_head_save | e_tail_save; }

a_e_wrap() { sed -e '$s/$/ \\/'; }

a_wrap() { sed -e '$s/$/\n /'; }

entity_save() { printf '%s\n' "${1}" | e_save; }

array_save() {
  for i; do
    entity_save "${i}" | a_e_wrap
  done |
    a_wrap
  unset i
}

array_append() {
  orig_name="${1}"
  shift
  new=$(array_save "${@}")
  eval "
    eval \"set -- \${${orig_name}}\"
    set -- \"\${@}\" ${new}
    ${orig_name}=\$(array_save \"\${@}\")
  "
  unset orig_name new
}

get_mtu() {
  output="${1}"
  existing_mtu="${2}"
  shift 2
  mtu_match=''
  dev_match=''
  mtu_match="$(printf %s "${output}" | sed -ne 's:^.*\<mtu \([0-9]\+\)\>.*$:\1:; t P; b; : P; p; q')"
  if [ -z "${mtu_match}" ]; then
    dev_match="$(printf %s "${output}" | sed -ne 's:^.*\<dev \([^]\+\)\>.*$:\1:; t P; b; : P; p; q')"
    [ -z "${dev_match}" ] ||
      mtu_match="$(ip link show dev "${dev_match}" | sed -ne 's:^.*\<mtu \([0-9]\+\)\>.*$:\1:; t P; b; : P; p; q')"
  fi
  if [ -n "${mtu_match}" ] &&
    [ "${mtu_match}" -gt "${existing_mtu}" ]; then
    printf %s "${mtu_match}"
  else
    printf %s "${existing_mtu}"
  fi
  unset output existing_mtu mtu_match dev_match
}

##

cmd() {
  printf '[#] %s\n' "${*}" >&2
  "${@}"
}

die() {
  printf '%s: %s\n' "${PROGRAM}" "${*}" >&2
  exit 1
}

parse_dnsmasq_restart() {
  if [ "${1}" = '--norestart' ]; then
    RESTART_DNSMASQ=0
  fi
}

parse_options() {
  interface_section=0
  line=''
  key=''
  value=''
  stripped=''
  v=''
  header_line=0
  CONFIG_FILE="${1}"
  parse_dnsmasq_restart "${2}"
  #shellcheck disable=SC2003
  ! expr match "${CONFIG_FILE}" '[a-zA-Z0-9_=+.-]\{1,15\}$' >/dev/null ||
    CONFIG_FILE="${CONFIG_FILE_BASE}/${CONFIG_FILE}.conf"
  [ -e "${CONFIG_FILE}" ] ||
    die "'${CONFIG_FILE}' does not exist"
  #shellcheck disable=SC2003
  expr match "${CONFIG_FILE}" '\(.*/\)\?\([a-zA-Z0-9_=+.-]\{1,15\}\)\.conf$' >/dev/null ||
    die 'The config file must be a valid interface name, followed by .conf'
  CONFIG_FILE="$(readlink "${CONFIG_FILE}")"
  if {
    stat_octal "${CONFIG_FILE}" || :
    stat_octal "$(printf %s "${CONFIG_FILE}" | sed -e 's:/[^/]*$::')" || :
  } 2>/dev/null | grep -vq '0$'; then
    printf 'Warning: `%s'\'' is world accessible\n' "${CONFIG_FILE}" >&2
  fi
  INTERFACE="$(printf %s "${CONFIG_FILE}" | sed -e 's:^\(.*/\)\?\([^/.]\+\)\.conf$:\2:')"
  while read -r line || [ -n "${line}" ]; do
    stripped="$(printf %s "${line}" | sed -e 's:#.*$::; /^[[:blank:]]*$/d')"
    key="$(printf %s "${stripped}" | sed -e 's#^[[:blank:]]*\([^=[:blank:]]\+\)[[:blank:]]*=.*$#\1#')"
    case "${key}" in
      '['*)
        if [ "${key}" = '[Interface]' ]; then
          interface_section=1
        else
          interface_section=0
        fi
        header_line=1
        ;;
      *)
        header_line=0
        ;;
    esac
    if [ "${header_line}" -eq 0 ] && [ "${interface_section}" -eq 1 ]; then
      value="$(
        printf %s "${stripped}" |
          sed -e 's#^[^=]\+=[[:blank:]]*\([^[:blank:]]\(.*[^[:blank:]]\)\?\)\?[[:blank:]]*$#\1#'
      )"
      case "$(printf %s "${key}" | tr '[:upper:]' '[:lower:]')" in
        address)
          #shellcheck disable=SC2046
          array_append ADDRESSES $(printf %s "${value}" | tr ',' ' ')
          continue
          ;;
        mtu)
          MTU="${value}"
          continue
          ;;
        dns)
          for v in $(printf %s "${value}" | tr ',' ' '); do
            #shellcheck disable=SC2003
            if expr match "${v}" '[0-9.]\+$' >/dev/null || expr match "${v}" '.*:.*$' >/dev/null; then
              array_append DNS "${v}"
            else
              array_append DNS_SEARCH "${v}"
            fi
          done
          continue
          ;;
        table)
          TABLE="${value}"
          continue
          ;;
        preup)
          array_append PRE_UP "${value}"
          continue
          ;;
        predown)
          array_append PRE_DOWN "${value}"
          continue
          ;;
        postup)
          array_append POST_UP "${value}"
          continue
          ;;
        postdown)
          array_append POST_DOWN "${value}"
          continue
          ;;
        saveconfig)
          read_bool SAVE_CONFIG "${value}"
          continue
          ;;
        *)
          :
          ;;
      esac
    fi
    WG_CONFIG="${WG_CONFIG:+${WG_CONFIG}${NL}}${line}"
  done <"${CONFIG_FILE}"
  unset interface_section line key value stripped v header_line
}

read_bool() {
  case "${2}" in
    true) eval "${1}=1" ;;
    false) eval "${1}=0" ;;
    *) die "'${2}' is neither true nor false" ;;
  esac
}

add_if() {
  ret=0
  if ! cmd ip link add "${INTERFACE}" type wireguard; then
    ret=${?}
    ! [ -e /sys/module/wireguard ] && type -p "${WG_QUICK_USERSPACE_IMPLEMENTATION:-wireguard-go}" >/dev/null ||
      exit "${ret}"
    printf '[!] Missing WireGuard kernel module. Falling back to slow userspace implementation.\n' >&2
    cmd "${WG_QUICK_USERSPACE_IMPLEMENTATION:-wireguard-go}" "${INTERFACE}"
  fi
  unset ret
}

del_if() {
  table=''
  fwmark=''
  [ "${HAVE_SET_DNS-0}" -eq 0 ] || unset_dns
  [ "${HAVE_SET_FIREWALL-0}" -eq 0 ] || remove_firewall
  #shellcheck disable=SC2003
  if [ -z "${TABLE}" ] ||
    [ "x${TABLE}" = 'xauto' ] &&
    get_fwmark table &&
    [[ "$(wg show "$INTERFACE" allowed-ips)" =~ "/0(\ |$'\n'|$)" ]]; then
    fwmark=$table
    table="$(printf "%d\n" $fwmark)"
    for proto in -4 -6; do
      while :; do
        case "$(ip "${proto}" rule show 2>/dev/null)" in
          *"not from all fwmark ${fwmark} lookup ${table}"*)
            cmd ip "${proto}" rule delete not from all fwmark "${fwmark}" lookup "${table}"
            ;;
          *)
            break
            ;;
        esac
      done
      while :; do
        case "$(ip "${proto}" rule show 2>/dev/null)" in
          *"from all lookup main suppress_prefixlength 0"*)
            cmd ip "${proto}" rule delete table main suppress_prefixlength 0
            ;;
          *)
            break
            ;;
        esac
      done
    done
    unset proto
  fi
  cmd ip link delete dev "${INTERFACE}"
  unset fwmark
  unset table
}

add_addr() {
  case "${1}" in
    *:*) proto=-6 ;;
    *) proto=-4 ;;
  esac
  cmd ip "${proto}" address add "${1}" dev "${INTERFACE}"
  unset proto
}

set_mtu_up() {
  mtu=1500
  endpoint=''
  v6_addr=''
  if [ -n "${MTU}" ]; then
    cmd ip link set mtu "${MTU}" dev "${INTERFACE}"
    cmd ifconfig "${INTERFACE}" up
  else
    cmd ip link set mtu $((mtu - 80)) dev "${INTERFACE}"
    cmd ifconfig "${INTERFACE}" up
  fi
  unset mtu endpoint v6_addr
}

#shellcheck disable=SC2120
set_dns() {
  DNS_CONFIG="${CONFIG_FILE_BASE}/dns/${INTERFACE}.conf"
  echo "interface=${INTERFACE}" > "${DNS_CONFIG}"
  eval "set -- ${DNS}"
  if [ ${#} -gt 0 ]; then
    DNS_CHAIN="wg-quick-${INTERFACE}-dns"
    iptables -D OUTPUT -j "${DNS_CHAIN}" >/dev/null 2>&1 || $(exit 0)
    iptables -L "${DNS_CHAIN}" >/dev/null 2>&1 && \
      iptables -F "${DNS_CHAIN}"  >/dev/null 2>&1 && \
      iptables -X "${DNS_CHAIN}" >/dev/null 2>&1
    cmd iptables -N "${DNS_CHAIN}"
    for NAMESERVER in ${@}; do
      echo "server=${NAMESERVER}" >> "${DNS_CONFIG}"
      cmd iptables -A "${DNS_CHAIN}" -i "${INTERFACE}" -p tcp --dst "${NAMESERVER}/32" --dport 53 -j ACCEPT
      cmd iptables -A "${DNS_CHAIN}" -i "${INTERFACE}" -p udp --dst "${NAMESERVER}/32" --dport 53 -j ACCEPT
    done
    cmd iptables -A "${DNS_CHAIN}" -i "${INTERFACE}" -p tcp --dport 53 -j REJECT
    cmd iptables -A "${DNS_CHAIN}" -i "${INTERFACE}" -p udp --dport 53 -j REJECT
    cmd iptables -A OUTPUT -j "${DNS_CHAIN}"
    HAVE_SET_DNS=1
  fi
  if [ $RESTART_DNSMASQ -eq 1 ]; then
    cmd service dnsmasq restart
  fi
}

unset_dns() {
  DNS_CONFIG="${CONFIG_FILE_BASE}/dns/${INTERFACE}.conf"
  eval "set -- ${DNS}"
  if [ ${#} -gt 0 ]; then
    DNS_CHAIN="wg-quick-${INTERFACE}-dns"
    cmd iptables -D OUTPUT -j "${DNS_CHAIN}" || $(exit 0)
    cmd iptables -F "${DNS_CHAIN}" || $(exit 0)
    cmd iptables -X "${DNS_CHAIN}" || $(exit 0)
    rm "$DNS_CONFIG"
    if [ $RESTART_DNSMASQ -eq 1 ]; then
      cmd service dnsmasq restart
    fi
  fi
}

add_route() {
  case "${1}" in
    *:*) proto=-6 ;;
    *) proto=-4 ;;
  esac
  if [ "${TABLE}" != off ]; then
    case "${TABLE}:${1}" in
      auto:*)
        cmd ip "${proto}" route add "${1}" dev "${INTERFACE}" table "${TABLE}"
        ;;
      # uncomment to enable adding default route
      #*:*/0)
      #  add_default "${1}"
        ;;
      *)
        [ -n "$(ip "${proto}" route show dev "${INTERFACE}" match "${1}" 2>/dev/null)" ] ||
          cmd ip "${proto}" route add "${1}" dev "${INTERFACE}"
        ;;
    esac
  fi
  unset proto
}

get_fwmark() {
  fwmark="$(wg show "${INTERFACE}" fwmark)" &&
    [ -n "${fwmark}" ] &&
    [ "x${fwmark}" != 'xoff' ] ||
    return 1
  eval "${1}=${fwmark}"
  unset fwmark
}

remove_firewall() {
  if type_p nft >/dev/null; then
    table=''
    nftcmd=''
    nft list tables 2>/dev/null | {
      while read -r table; do
        case "${table}" in
          *" wg-quick-${INTERFACE}")
            nftcmd="${nftcmd:+${nftcmd}${NL}}delete ${table}"
            ;;
          *)
            :
            ;;
        esac
      done
      if [ -n "${nftcmd}" ]; then
        printf '%s\n' "${nftcmd}" |
          cmd nft -f
      fi
    }
    unset table nftcmd
  fi
  if type_p iptables >/dev/null; then
    iptables=''
    for iptables in iptables ip6tables; do
      "${iptables}-save" 2>/dev/null | {
        restore=''
        found=0
        line=''
        while read -r line; do
          case "${line}" in
            \** | COMMIT | '-A '*'-m comment --comment "wg-quick(8) rule for '"${INTERFACE}"'"'*)
              case "${line}" in
                -A*)
                  found=1
                  ;;
                *)
                  :
                  ;;
              esac
              restore="${restore:+${restore}${NL}}-D${line#-A}"
              ;;
            *)
              :
              ;;
          esac
        done
        [ "${found}" -ne 1 ] ||
          printf '%s\n' "${restore}" |
          cmd "${iptables}-restore" -n
        unset restore found line
      }
    done
    unset iptables
  fi
}

add_default() {
  table=''
  line=''
  proto=''
  iptables=''
  pf=''
  restore=''
  nftable=''
  nftcmd=''
  if ! get_fwmark table; then
    table=51820
    while [ -n "$(ip -4 route show table "${table}" 2>/dev/null)" ] ||
      [ -n "$(ip -6 route show table "${table}" 2>/dev/null)" ]; do
      table=$((table + 1))
    done
    cmd wg set "${INTERFACE}" fwmark "${table}"
  fi
  case "${1}" in
    *:*)
      proto='-6'
      iptables='ip6tables'
      pf='ip6'
      ;;
    *)
      proto='-4'
      iptables='iptables'
      pf='ip'
      ;;
  esac
  cmd ip "${proto}" route add "${1}" dev "${INTERFACE}" table "${table}"
  cmd ip "${proto}" rule add not fwmark "${table}" table "${table}"
  cmd ip "${proto}" rule add table main suppress_prefixlength 0

  restore="*raw${NL}"
  nftable="wg-quick-${INTERFACE}"
  nftcmd="${nftcmd:+${nftcmd}${NL}}add table ${pf} ${nftable}"
  nftcmd="${nftcmd:+${nftcmd}${NL}}add chain ${pf} ${nftable} preraw { type filter hook prerouting priority -300; }"
  nftcmd="${nftcmd:+${nftcmd}${NL}}add chain ${pf} ${nftable} premangle { type filter hook prerouting priority -150; }"
  nftcmd="${nftcmd:+${nftcmd}${NL}}add chain ${pf} ${nftable} postmangle { type filter hook postrouting priority -150; }"
  ip -o "${proto}" addr show dev "${INTERFACE}" 2>/dev/null | {
    match=''
    while read -r line; do
      match="$(
        printf %s "${line}" |
          sed -ne 's/^.*inet6\? \([0-9a-f:.]\+\)\/[0-9]\+.*$/\1/; t P; b; : P; p'
      )"
      [ -n "${match}" ] ||
        continue
      restore="${restore:+${restore}${NL}}-I PREROUTING ! -i ${INTERFACE} -d ${match} -m addrtype ! --src-type LOCAL -j DROP"
      nftcmd="${nftcmd:+${nftcmd}${NL}}add rule ${pf} ${nftable} preraw iifname != \"${INTERFACE}\" ${pf} daddr ${match} fib saddr type != local drop"
    done
    restore="${restore:+${restore}${NL}}COMMIT${NL}*mangle${NL}-I POSTROUTING -m mark --mark ${table} -p udp -j CONNMARK --save-mark${NL}-I PREROUTING -p udp -j CONNMARK --restore-mark${NL}COMMIT"
    nftcmd="${nftcmd:+${nftcmd}${NL}}add rule ${pf} ${nftable} postmangle meta l4proto udp mark ${table} ct mark set mark"
    nftcmd="${nftcmd:+${nftcmd}${NL}}add rule ${pf} ${nftable} premangle meta l4proto udp meta mark set ct mark"
    ! [ "${proto}" = '-4' ] ||
      echo 1 > /proc/sys/net/ipv4/conf/all/src_valid_mark
    if type_p nft >/dev/null; then
      printf '%s\n' "${nftcmd}" |
        cmd nft -f
    else
      printf '%s\n' "${restore}" |
        cmd "${iptables}-restore" -n
    fi
    unset match
  }
  HAVE_SET_FIREWALL=1
  unset table line proto iptables pf restore nftable nftcmd
}

set_config() {
  if [ -e /dev/stdin ]; then
    printf '%s\n' "${WG_CONFIG}" |
      cmd wg setconf "${INTERFACE}" /dev/stdin
  else
    tempfile="$(mktemp)"
    exit_trap push "rm -f \"${tempfile}\""
    printf '%s\n' "${WG_CONFIG}" >"${tempfile}"
    cmd wg setconf "${INTERFACE}" "${tempfile}"
    rm -f "${tempfile}"
    exit_trap pop
    unset tempfile
  fi
}

save_config() {
  old_umask=''
  new_config=''
  current_config=''
  address=''
  cmd=''
  addr_match="$(
    ip -all -oneline address show dev ${INTERFACE} |
      awk -F "[, ]+" '{print $4}'
  )"
  new_config='[Interface]'
  for address in ${addr_match}; do
    new_config="${new_config:+${new_config}${NL}}Address = ${address}"
  done
  cat "/etc/wireguard/dns/${INTERFACE}.conf" 2>/dev/null | {
    while read -r address; do
      addr_match="$(
        printf %s "${address}" |
          sed -ne 's#^server=\([a-zA-Z0-9_=+:%.-]\+\)$#\1#; t P; b; : P; p'
      )"
      [ -z "${addr_match}" ] ||
        new_config="${new_config:+${new_config}${NL}}DNS = ${addr_match}"
    done
    if [ -n "${MTU}" ]; then
      mtu_match="$(
        ip link show dev "${INTERFACE}" |
          sed -ne 's/^.*mtu \([0-9]\+\).*$/\1/; t P; b; : P; p'
      )"
      [ -z "${mtu_match}" ] ||
        new_config="${new_config:+${new_config}${NL}}MTU = ${mtu_match}"
    fi
    [ -z "${TABLE}" ] ||
      new_config="${new_config:+${new_config}${NL}}Table = ${TABLE}"
    [ "${SAVE_CONFIG}" -eq 0 ] ||
      new_config="${new_config:+${new_config}${NL}}SaveConfig = true"
    eval "set -- ${PRE_UP}"
    for cmd; do
      new_config="${new_config:+${new_config}${NL}}PreUp = ${cmd}"
    done
    eval "set -- ${POST_UP}"
    for cmd; do
      new_config="${new_config:+${new_config}${NL}}PostUp = ${cmd}"
    done
    eval "set -- ${PRE_DOWN}"
    for cmd; do
      new_config="${new_config:+${new_config}${NL}}PreDown = ${cmd}"
    done
    eval "set -- ${POST_DOWN}"
    for cmd; do
      new_config="${new_config:+${new_config}${NL}}PostDown = ${cmd}"
    done
    old_umask="$(umask)"
    umask 077
    current_config="$(cmd wg showconf "${INTERFACE}")"
    exit_trap push "rm -f \"${CONFIG_FILE}.tmp\""
    printf '%s\n' "${current_config}" |
      sed -e "s#\\[Interface\\]\$#$(
        printf %s "${new_config}" |
          sed -e '$!s/$/\\n/' |
          tr -d '\n'
      )#" >"${CONFIG_FILE}.tmp" ||
      die 'Could not write configuration file'
    sync "${CONFIG_FILE}.tmp"
    mv "${CONFIG_FILE}.tmp" "${CONFIG_FILE}" ||
      die 'Could not move configuration file'
    exit_trap pop
    umask "${old_umask}"
    unset new_config current_config old_umask cmd mtu_match addr_match address
  }
}

execute_hooks() {
  for hook; do
    hook="$(
      printf %s "${hook}" |
        sed -e "s^%i^${INTERFACE}^g"
    )"
    printf '[#] %s\n' "${hook}" >&2
    (eval "${hook}")
  done
  unset hook
}

cmd_usage() {

	echo "Usage: $PROGRAM [ up | down | save | strip ] [ CONFIG_FILE | INTERFACE ]"
	echo "  CONFIG_FILE is a configuration file, whose filename is the interface name"
	echo "  followed by '.conf'. Otherwise, INTERFACE is an interface name, with"
	echo "  configuration found at /etc/wireguard/INTERFACE.conf. It is to be readable"
	echo "  by wg(8)'s 'setconf' sub-command, with the exception of the following additions"
	echo "  to the [Interface] section, which are handled by $PROGRAM:"
	echo "  - Address: may be specified one or more times and contains one or more"
	echo "    IP addresses (with an optional CIDR mask) to be set for the interface."
	echo "  - DNS: an optional DNS server to use while the device is up."
	echo "  - MTU: an optional MTU for the interface; if unspecified, auto-calculated."
	echo "  - Table: an optional routing table to which routes will be added; if"
	echo "    unspecified or 'auto', the default table is used. If 'off', no routes"
	echo "    are added."
	echo "  - PreUp, PostUp, PreDown, PostDown: script snippets which will be executed"
	echo "    by bash(1) at the corresponding phases of the link, most commonly used"
	echo "    to configure DNS. The string '%i' is expanded to INTERFACE."
	echo "  - SaveConfig: if set to 'true', the configuration is saved from the current"
	echo "    state of the interface upon shutdown."
	echo "See wg-quick(8) for more info and examples."

}

cmd_up() {
  i=''
  [ -z "$(ip link show dev "${INTERFACE}" 2>/dev/null)" ] ||
    die "'${INTERFACE}' already exists"
  exit_trap push 'del_if'
  eval "execute_hooks ${PRE_UP}"
  add_if
  set_config
  set_mtu_up
  eval "set -- ${ADDRESSES}"
  for i; do
    add_addr "${i}"
  done
  set_dns
  for i in $(
    wg show "${INTERFACE}" allowed-ips |
      while read -r _ j; do
        for k in ${j}; do
          #shellcheck disable=SC2003
          ! expr match "${k}" '[0-9a-z:.]\+/[0-9]\+$' >/dev/null ||
            printf '%s\n' "${k}"
        done
      done |
      sort -nr -k 2 -t /
    unset j k
  ); do
    add_route "${i}"
  done
  eval "execute_hooks ${POST_UP}"
  unset i
  exit_trap pop
}

cmd_down() {
  case " $(wg show interfaces) " in
    *" ${INTERFACE} "*) : ;;
    *) die "'${INTERFACE}' is not a WireGuard interface" ;;
  esac
  eval "execute_hooks ${PRE_DOWN}"
  [ "${SAVE_CONFIG}" -eq 0 ] ||
    save_config
  del_if
  unset_dns || :
  remove_firewall || :
  eval "execute_hooks ${POST_DOWN}"
}

cmd_save() {
  case " $(wg show interfaces) " in
    *" ${INTERFACE} "*) : ;;
    *) die "'${INTERFACE}' is not a WireGuard interface" ;;
  esac
  save_config
}

cmd_strip() { printf '%s\n' "${WG_CONFIG}"; }

##

EXIT_TRAP=''
LC_ALL=C
SELF="$(readlink "${0}")"
PATH="$(printf %s "${SELF}" | sed -e 's:/[^/]*$::'):${PATH}"
export LC_ALL PATH
[ -n "${CONFIG_FILE_BASE}" ] ||
  CONFIG_FILE_BASE='/etc/wireguard'
NL='
'
WG_CONFIG=''
INTERFACE=''
ADDRESSES=$(array_save)
MTU=''
DNS=$(array_save)
DNS_SEARCH=$(array_save)
TABLE=''
PRE_UP=$(array_save)
POST_UP=$(array_save)
PRE_DOWN=$(array_save)
POST_DOWN=$(array_save)
SAVE_CONFIG=0
CONFIG_FILE=''
PROGRAM="$(printf %s "${0}" | sed -e 's:^.*/\([^/]*\)$:\1:')"
ARGS=$(array_save "${@}")
RESTART_DNSMASQ=1
HAVE_SET_DNS=0
HAVE_SET_FIREWALL=0

# ~~ ensure directories exist and module loaded ~~

if [ ! -d "$CONFIG_FILE_BASE" ]; then
  mkdir "$CONFIG_FILE_BASE"
  chmod 700 "$CONFIG_FILE_BASE"
fi
if [ ! -d "$CONFIG_FILE_BASE/dns" ]; then
  mkdir "$CONFIG_FILE_BASE/dns"
  chmod 700 "$CONFIG_FILE_BASE/dns"
fi

lsmod | grep wireguard 1>/dev/null || modprobe wireguard

# ~~ function override insertion point ~~

case "${#}:${1}" in
  1:--help | 1:-h | 1:help)
    cmd_usage
    ;;
  2:up | 2:down | 2:save | 2:strip | 3:up | 3:down)
    parse_options "${2}" "${3}"
    case "${1}" in
      up)
        cmd_up
        ;;
      down)
        cmd_down
        ;;
      save)
        cmd_save
        ;;
      strip)
        cmd_strip
        ;;
      *)
        :
        ;;
    esac
    ;;
  *)
    cmd_usage
    exit 1
    ;;
esac
