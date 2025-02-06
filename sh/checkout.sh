#!/bin/bash
set -e

if [ -z "${1+x}" ]; then
    echo "err: specify the assignment variable ('latest' to return to latest)"
    exit 1
fi

if [ "$1" = "latest" ]; then
    git checkout main
    exit 0
fi

current=$(git rev-parse HEAD)
git checkout main
if [ ! -f "commits/assignment$1.txt" ]; then
    echo "err: no commit log for assignment $1"
    git checkout $current
    exit 1
fi
git checkout $(cat commits/assignment$1.txt)