#!/bin/bash
set -e

if [ -z "${1+x}" ]; then
    echo "err: specify the assignment variable"
    exit 1
fi

if [ ! -f "readmes/assignment$1.txt" ]; then
    echo "err: assignment$1 has no readme"
    exit 1
fi

echo "creating package directory"
decimal_name=$(echo "$1" | sed -r 's/_+/./g')
package_dir="senneff_camden.assignment-${decimal_name}"
if [ -d "$package_dir" ]; then
    echo "cleaning up existing package directory"
    rm -r "$package_dir"
fi

mkdir $package_dir
cp "readmes/assignment$1.txt" $package_dir/README
cp -r src $package_dir
cp Makefile $package_dir
cp CHANGELOG $package_dir
cp -r assets $package_dir

echo "testing compile"
cd $package_dir
make -s
make -s clean
cd ..

echo "creating tar"
tar cvfz "$package_dir.tar.gz" "$package_dir" 1> /dev/null

echo "removing package directory"
rm -r "$package_dir"

echo "testing tar"
if [ -d "test" ]; then
    echo "cleaning up existing test directory"
    rm -r "test"
fi
mkdir test
cd test
gzip -dc "../$package_dir.tar.gz" | tar -xvf - 1> /dev/null
cd "$package_dir"
make -s
make -s clean
cd ../..
rm -r test

echo "ready!"
