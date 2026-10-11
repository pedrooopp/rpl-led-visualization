#!/usr/bin/env bash
# Copies the modified Contiki-NG files of this project into a Contiki-NG tree.
#
# Usage: contiki-ng-patches/apply-patches.sh <path-to-contiki-ng> [lite|classic]
#
# - The RPL implementation defaults to "lite" (Contiki-NG's default).
# - Every file that gets replaced is first saved next to it as <file>.orig
#   (only once, so re-running the script never overwrites your backup).
# - Tested against Contiki-NG release v4.9.
set -euo pipefail

CONTIKI_DIR="${1:-}"
RPL="${2:-lite}"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(dirname "$HERE")"

if [[ -z "$CONTIKI_DIR" || ! -f "$CONTIKI_DIR/Makefile.include" ]]; then
  echo "Usage: $0 <path-to-contiki-ng> [lite|classic]" >&2
  exit 1
fi
if [[ "$RPL" != "lite" && "$RPL" != "classic" ]]; then
  echo "RPL mode must be 'lite' or 'classic' (got '$RPL')" >&2
  exit 1
fi
CONTIKI_DIR="$(cd "$CONTIKI_DIR" && pwd)"

# The patched sources #include "compartido/proyecto-rsi-grupo-3/cliente/variables_compartidas.h",
# so the project has to live at <contiki-ng>/compartido/proyecto-rsi-grupo-3.
EXPECTED="$CONTIKI_DIR/compartido/proyecto-rsi-grupo-3"
if [[ "$(cd "$EXPECTED" 2>/dev/null && pwd -P)" != "$(cd "$REPO_DIR" && pwd -P)" ]]; then
  echo "Warning: this repository is not at $EXPECTED" >&2
  echo "         The patched files include the shared header from that path; the build" >&2
  echo "         will fail unless you move/clone the repository there." >&2
fi

apply() {
  local rel="$1" src="$HERE/$1" dst="$CONTIKI_DIR/$1"
  [[ -f "$dst" ]] || { echo "Missing in Contiki-NG tree: $rel" >&2; exit 1; }
  [[ -f "$dst.orig" ]] || cp "$dst" "$dst.orig"
  cp "$src" "$dst"
  echo "patched  $rel"
}

while IFS= read -r -d '' f; do
  rel="${f#"$HERE"/}"
  case "$rel" in
    os/net/routing/rpl-lite/*)    [[ "$RPL" == "lite"    ]] || continue ;;
    os/net/routing/rpl-classic/*) [[ "$RPL" == "classic" ]] || continue ;;
  esac
  apply "$rel"
done < <(find "$HERE" -type f \( -name '*.c' -o -name '*.h' \) -print0 | sort -z)

echo "Done (RPL-$RPL). To restore the originals, copy each *.orig file back over its patched file."
