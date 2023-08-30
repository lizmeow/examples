#/bin/bash -x

TEST_DIR=test_set
N_TESTS=15
ERROR_FLAG=0

make

for i in $(seq 1 $N_TESTS);
do
    ./reassemble $TEST_DIR/test$i.txt > $TEST_DIR/test$i.out 2>&1
    diff $TEST_DIR/test$i.out $TEST_DIR/test$i.ref
    if [ $? -ne 0 ]; then
        printf "$TEST_DIR/test$i.txt input file did not pass.\n"
        ERROR_FLAG=1
    fi
done

if [ $ERROR_FLAG -ne 0 ]; then
    printf "Not all tests passed.\n"
else
    printf "All tests passed.\n"
fi