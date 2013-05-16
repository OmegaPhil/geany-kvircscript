#!/bin/bash


# Making sure the passed argument is a path to a script. Note that geany
# does not have an option to pass the full path of the script in one -
# however the working directory is set appropriately
[[ ! -f $1 || ! $1 =~ .+\.kvs ]] && {
    
    # It isn't - warning user and exiting
    echo "The passed path '$1' is not a valid script" > /dev/stderr
    zenity --error --title "KVIrc Script Loader" --text "The passed path '$1' is not a valid script"
    exit 1
}

# Debug code
#zenity --info --text="$(pwd)"

# Parsing script with KVIrc - even if the script fails to parse, KVIrc
# does not reflect his in the return code!!
/usr/local/bin/kvirc -x "parse \"$(pwd)/$1\""
/usr/local/bin/kvirc -x "echo \"KVIrc script parsing of '$1' complete\""
/usr/local/bin/kvirc -x "options.save"
