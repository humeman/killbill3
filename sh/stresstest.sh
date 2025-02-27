#!/bin/bash

while [ 1 -eq 1 ]; do
	valgrind ./assignment1_03
	if [ $? -ne 0 ]; then
		echo "err $?"
		exit 1
	fi
done
