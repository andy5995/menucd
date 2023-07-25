# menucd
Script that presents a menu of directories at the command line

## Requirements

* [dialog](https://invisible-island.net/dialog/dialog.html)

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


