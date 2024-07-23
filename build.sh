#/bin/bash
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

if [ "$1" == "-t" ] || [ "$1" == "--tests" ]; then
    make all_tests
else
    make all
fi