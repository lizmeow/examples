#!/bin/bash
# Concatenate variable string "s" to output 5000 times.
# Using echo in a loop will inject a newline separator.
# For example
# ./t.sh > t.out

s={an}

for i in {1..5000}; do
    echo $s
done