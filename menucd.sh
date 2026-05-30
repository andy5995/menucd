#!/usr/bin/env -S bash --posix
#
#~ This is free and unencumbered software released into the public domain.

#~ Anyone is free to copy, modify, publish, use, compile, sell, or
#~ distribute this software, either in source code form or as a compiled
#~ binary, for any purpose, commercial or non-commercial, and by any
#~ means.

#~ In jurisdictions that recognize copyright laws, the author or authors
#~ of this software dedicate any and all copyright interest in the
#~ software to the public domain. We make this dedication for the benefit
#~ of the public at large and to the detriment of our heirs and
#~ successors. We intend this dedication to be an overt act of
#~ relinquishment in perpetuity of all present and future rights to this
#~ software under copyright law.

#~ THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#~ EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#~ MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#~ IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
#~ OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
#~ ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
#~ OTHER DEALINGS IN THE SOFTWARE.

#~ For more information, please refer to <https://unlicense.org>

VERSION="0.2.3"
SAVE_FILE="${HOME}/.menucd-save"
SAVE_SEP_HEAD="----- saved ----"
SAVE_SEP_FOOT="----------------"
SHOW_ALL="[ show all ]"

echo "menucd $VERSION"
echo "save-file location: ${HOME}/.menucd-save"
if [ "$1" = "-v" ]; then
  exit 0
fi

if [ -z "$(command -v dialog)" ]; then
  echo "The dialog package is required"
  echo "(See https://invisible-island.net/dialog/dialog.html)"
  exit 1
fi

if [ ! -w "${SAVE_FILE}" ]; then
  echo "Creating ${SAVE_FILE}"
  touch "${SAVE_FILE}" || exit 1
fi

filter=""
while :; do
  mapfile -t curdir < <(find . -maxdepth 1 -mindepth 1 -type d -printf '%f\n' | LC_ALL=C sort)
  mapfile -t saved < "${SAVE_FILE}"
  i=0

  if [ "$PWD" != "/" ]; then
    options=("$i" "..")
  else
    options=("" "")
  fi

  if [ -n "$filter" ]; then
    ((i=i+1))
    options+=("$i" "$SHOW_ALL")
  fi

  for dir in "$SAVE_SEP_HEAD" "${saved[@]}" "$SAVE_SEP_FOOT" "${curdir[@]}"; do
    if [ -z "$dir" ]; then
      continue
    fi
    if [ -n "$filter" ] \
      && [ "$dir" != "$SAVE_SEP_HEAD" ] && [ "$dir" != "$SAVE_SEP_FOOT" ] \
      && [[ "${dir,,}" != *"${filter,,}"* ]]; then
      continue
    fi
    ((i=i+1))
    options+=("$i" "$dir")
  done

  title="PWD:\n$PWD"
  if [ -n "$filter" ]; then
    title="${title}\nfilter: $filter"
  fi

  while :; do
    cmd=(dialog \
      --ok-label "CD" \
      --extra-button --extra-label "Save PWD" \
      --help-button --help-label "Search" \
      --keep-tite \
      --cancel-label "Quit to PWD" \
      --menu "$title" -1 -1 16)
    choices=$("${cmd[@]}" "${options[@]}" 2>&1 >/dev/tty)
    ret=$?
    selected=""
    if [ "$ret" -eq 0 ] && [ -n "$choices" ]; then
      selected="${options[$choices*2+1]}"
      if [ "$selected" = "$SAVE_SEP_HEAD" ] || [ "$selected" = "$SAVE_SEP_FOOT" ]; then
        continue
      fi
    fi
    break
  done

  if  [ "$ret" -eq 255 ]; then
    exit 0
  elif [ "$ret" -eq 1 ]; then
    echo "$PWD" > /tmp/menucd.cd.exit
    exit 0
  elif [ "$ret" -eq 2 ]; then
    filter=$(dialog --keep-tite \
      --inputbox "Filter the current listing (empty clears it):" -1 -1 "$filter" \
      2>&1 >/dev/tty)
  elif [ "$ret" -eq 3 ]; then
    echo "${PWD}" >> "${SAVE_FILE}"
  elif [ "$selected" = "$SHOW_ALL" ]; then
    filter=""
  else
    cd "$selected" || exit $?
  fi
done
exit 0
