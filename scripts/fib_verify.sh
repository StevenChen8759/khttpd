#!/usr/bin/env bash

#rm -rf fibnum.txt

for i in {5824..10000}
do
    echo $i
    wget -q -O - 127.0.0.1:8081/fib/$i >> fibnum.txt
    echo "" >> fibnum.txt
done

#echo "Fibonacci number get finished..."

# Run specific C file to verify correctness of response fibonacci number
diff fibnum.txt ./scripts/fibnum_crawlar.txt
