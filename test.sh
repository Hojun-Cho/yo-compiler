#!/bin/sh

./com ./test/fib && cp exported obj ./test/fib/
./com ./test/nqueen && cp exported obj ./test/nqueen/
./com ./test && ./vm/run obj
