#!/bin/bash

if [ $# -ne 2 ]; then
	echo "Correct format: ./systemInit.sh TARGET_FOLDER SOURCE_FOLDER"
	exit
fi

target_folder=$(realpath "$1")
source_folder=$(realpath "$2")

hashes () {
	item=$1
  	hash=$(shasum -a 256 "$item" | head -c 16)
	chars=$(echo "$hash" | grep -o '.')
	path=""
	for char in $chars; do
		path="$path/$char"
		mkdir -p "$target_folder$path"
	done

  	cp "$item" "$target_folder/$path/$hash"

  	echo "$hash" > "$item"
}

find $source_folder -type f | while read file; do hashes $file; done
