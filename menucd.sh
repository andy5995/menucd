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

VERSION="0.2.2"
SAVE_FILE="${HOME}/.menucd-save"

if [ -z "$(command -v dialog)" ]; then
  echo "The dialog package is required"
  echo "(See https://invisible-island.net/dialog/dialog.html)"
  exit 1
fi
touch "${SAVE_FILE}"

while :; do
  curdir=$(find . -maxdepth 1 -type d -printf '%fx0x0x0x0\n' | sort)
  curdir=$(echo "$curdir" | sed 's/ /%20/g')
  curdir=$(echo "$curdir" | sed 's/x0x0x0x0/ /g')
  curdir=$(echo "${curdir}" | sed 's/^\./00000000/g')
  curdir=$(echo "${curdir}" | sort)
  curdir=$(echo "${curdir}" | sed 's/00000000/\./g')
  i=0
  dirs=("..")
  options=($i "$dirs")

  for dir in ${curdir} `cat ${SAVE_FILE}`; do
    if [ "$dir" = "." ]; then
      continue
    fi
    let i=i+1
    dir=$(echo "${dir}" | sed 's/%20/ /g')
    dirs+=("${dir}")
    options+=($i "$dir")
  done

  cmd=(dialog --extra-button --extra-label "Save" --keep-tite --cancel-label "Quit" --menu $(pwd) -1 -1 16)
  choices=$("${cmd[@]}" "${options[@]}" 2>&1 >/dev/tty)
  ret=$?

  if  [ $ret -eq 255 ]; then
    exit 0
  elif [ $ret -eq 1 ]; then
    echo "$PWD" > /tmp/menucd.cd.exit
    exit 0
  elif [ $ret -eq 3 ]; then
    echo "${PWD}" >> "${SAVE_FILE}"
  else
    cd ${options[(($choices+1)*2)-1]}
  fi
done
exit 0