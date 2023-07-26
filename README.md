[![run shellcheck](https://github.com/andy5995/menucd/actions/workflows/shellcheck.yml/badge.svg)](https://github.com/andy5995/menucd/actions/workflows/shellcheck.yml)

# menucd
Directory browser and changer for the command line

## Requirements

* [dialog](https://invisible-island.net/dialog/dialog.html)

Linux, unix, BSD, MacOS

This script probably won't work from the Windows command line unless you're
using WSL or some other modified command line environment.

## Usage

Add this function to your environment by editing your ~/.profile, ~/.bashrc,
~/.zshrc, etc (change the path in the code below to match the path to where
the `menucd.sh` script is located):

```sh
function menucd () {
  $HOME/scripts/menucd.sh $@
  ret=$?
  if [ -r /tmp/menucd.cd.exit ]; then
          cd "`cat /tmp/menucd.cd.exit`"
          rm /tmp/menucd.cd.exit
  elif [ $ret != 0 ]; then
          echo Fail
  fi
}
```

Reload `~/.profile` (or whichever rc file you edited):

    source ~/.profile

Then run `menucd`. If you run the script (`menucd.sh`) by itself, it won't work.

