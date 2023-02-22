#!/bin/bash

if [ $# -ne 1 ]; then
	echo "Correct format: ./restoreFiles.sh SOURCE"
	exit
fi

source_path=$(realpath "$1")
data_path="$source_path/comiData"
files_path="$source_path/files"

retrieveContent () {
    item=$1
    hash=$(cat "$item")
	chars=$(echo "$hash" | grep -o '.')
	path=""
	for char in $chars; do
		path="$path/$char"
	done

  	cat "$data_path/$path/$hash" > "$item"
}

find $files_path -type f | while read file; do retrieveContent $file; done
