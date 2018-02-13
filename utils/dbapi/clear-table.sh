#!/bin/sh

######################
# AUTHOR: zengfanfan #
######################

db_dir=/cfg/database
db_name=default
sqlopts=
full_path=0

if [ ! "${1:0:1}" == "-" ]; then
    table_name=$1
    shift
fi

#
# table_exists: check if table exists
#

table_exists()
{
    for i in $(sqlite3 "$db_file" ".tables"); do
        if [ $i == $1 ]; then
            return 0
        fi
    done
    return 1
}

#
# set_db_file: choose main or backup database
#

set_db_file()
{
    if [ $full_path == 0 ]; then
        db_file=$db_dir/$db_name.db
    fi
}

help()
{
    echo -e "Usage: ${0##*/} <table-name> [option]"
    echo
    echo 'Available options:'
    echo '  -h        Display this information.'
    echo '  -D        Specify database name.'
    echo '  -f        Specify database file (full path).'
    echo '  -A        Append sqlite3 options.'
    echo
    set_db_file
    if [ ! -z $db_file ]; then
        echo 'Available tables:'
        for i in $(sqlite3 "$db_file" ".tables"); do
            echo "  $i"
        done
        echo
    fi
    exit 1
}

while getopts 'D:f:A:h' opt; do
    case $opt in
        D) db_name=$OPTARG;;
        f) db_file=$OPTARG; full_path=1;;
        A) sqlopts=$OPTARG;;
        h) help;;
    esac
done

shift $(($OPTIND - 1))
if [ $# -gt 0 ]; then
    table_name=$1
fi


if [ -z $db_name ] || [ -z $table_name ]; then
    help
fi

#
# execute sql statement
#

set_db_file

sqlite3 "$db_file" "DELETE FROM \`$table_name\`;" $sqlopts

