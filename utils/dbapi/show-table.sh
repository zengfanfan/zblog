#!/bin/sh

######################
# AUTHOR: zengfanfan #
######################

db_dir=/cfg/database
db_name=default
postfix="-header -column"
width="4 3"
sqlopts=
raw=0
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
    echo '  -d        Specify database name.'
    echo '  -D        Specify raw database name, regardless of backup mechanism.'
    echo '  -f        Specify database file (full path).'
    echo "  -w        Widths of columns, separate by comma(,)"
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

while getopts 'd:D:f:w:A:h' opt; do
    case $opt in
        d) db_name=$OPTARG; raw=0;;
        D) db_name=$OPTARG; raw=1;;
        f) db_file=$OPTARG; full_path=1;;
        A) sqlopts=$OPTARG;;
        w) width=`echo $OPTARG|sed 's/,/ /g'`;;
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

table_exists $table_name && echo -n "Found: "
table_exists $table_name && sqlite3 "$db_file" "SELECT COUNT(*) FROM \`$table_name\`;"
sqlite3 "$db_file" "SELECT * FROM \`$table_name\`;" $sqlopts $postfix -cmd ".width $width"

