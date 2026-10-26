#!/bin/bash
# Prints the HELL banner (env/banner.txt) with a flame gradient when the terminal supports it.
# Used by env/setup.sh (skip with HELL_QUIET=1) and by `hell` without arguments.
hell_banner() {
  local dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
  local file="$dir/banner.txt"
  case "${LC_ALL:-${LC_CTYPE:-${LANG:-}}}" in *UTF-8*|*utf8*|*utf-8*) ;; *) file="$dir/banner_ascii.txt" ;; esac
  local color=1
  [ -t 1 ] || color=0
  [ -n "${NO_COLOR:-}" ] && color=0
  [ "${TERM:-dumb}" = "dumb" ] && color=0
  if [ "$color" = 1 ]; then
    # flames: gradient by row (pale yellow at the sparks to orange at the letters); letters: red shades
    local flame=(230 229 228 227 226 226 220 220 214 214 208 208 202 202 202 202 202 202 202 202 202 202 202 202 202 196 196 196)
    local red=(196 196 196 196 196 196 160 160 160 160 124 124 88)
    local i=0 line fc lc
    while IFS= read -r line; do
      fc=${flame[$((i < ${#flame[@]} ? i : ${#flame[@]} - 1))]}
      lc=${red[$(( i >= 12 ? (i - 12 < ${#red[@]} ? i - 12 : ${#red[@]} - 1) : 0 ))]}
      printf '%s\n' "$line" | sed -E "s/([█▓#=]+)/$(printf '\033[1;38;5;%sm' "$lc")\\1$(printf '\033[1;38;5;%sm' "$fc")/g; s/^/$(printf '\033[1;38;5;%sm' "$fc")/; s/\$/$(printf '\033[0m')/"
      i=$((i + 1))
    done < "$file"
  else
    cat "$file"
  fi
}
