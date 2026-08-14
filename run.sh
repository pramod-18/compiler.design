#!/bin/bash

set -e

echo "Building lexer..."
make clean && make
echo "Build successful."
echo "=============================="

for testfile in tests/lexer_tests/*.txt; do
    [ -e "$testfile" ] || continue
    
    echo "Running test: $(basename "$testfile")"
    echo "------------------------------"
    
    if [ -f "./src/lex" ]; then
        ./src/lex < "$testfile"
    else
        echo "Error: Lexer executable not found in ./src/"
        exit 1
    fi
    
    echo "------------------------------"
done