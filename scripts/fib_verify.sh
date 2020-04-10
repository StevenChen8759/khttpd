#!/usr/bin/env bash

rm -rf fibnum.txt

for i in {0..1000}
do
    wget -q -O - 127.0.0.1:8081/fib/$i >> fibnum.txt
    echo "" >> fibnum.txt
done

echo "Fibonacci number get finished..."

# Run specific C file to verify correctness of response fibonacci number

