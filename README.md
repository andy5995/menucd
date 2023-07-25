# menucd
Script that presents a menu of directories at the command line

## Requirements

* [dialog](https://invisible-island.net/dialog/dialog.html)

Linux, unix, BSD, MacOS

This script probably won't work from the Windows command line unless you're
using WSL or some other modified command line environment.

## Usage

Save the script to a file, and then add this function to your environment by
editing your ~/.profile, ~/.bashrc, ~/.zshrc, etc (edit the path to the script
below to match the path to where you saved the script above):

```sh
function menucd () {
        $HOME/scripts/menucd.sh
        if [ -r /tmp/menucd.cd.exit ]; then
                cd "`cat /tmp/menucd.cd.exit`"
                rm /tmp/menucd.cd.exit
        else
                echo Fail
        fi
}
```

Reload `~/.profile` (or whichever rc file you edited):

    source ~/.profile

Then run `menucd`. If you run the script (`menucd.sh`) by itself, it won't work.

