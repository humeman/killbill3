#!/bin/bash

while [ 1 -eq 1 ]; do
	./assignment1_1 -b
	if [ $? -ne 0 ]; then
		echo "err $?"
		exit 1
	fi
done
