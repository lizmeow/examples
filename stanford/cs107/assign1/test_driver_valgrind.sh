#/bin/bash -x

TEST_DIR=test_set
N_TESTS=15
ERROR_FLAG=0
KEY_PHRASE="All heap blocks were freed -- no leaks are possible"

make clean
make -f Makefile.debug

for i in $(seq 1 $N_TESTS);
do
    r=$(valgrind ./reassemble $TEST_DIR/test$i.txt 2>&1)
    echo "$r" | grep -q "$KEY_PHRASE" 
    if [ $? -ne 0 ]; then
        printf "$TEST_DIR/test$i.txt did not pass valgrind.\n"
        ERROR_FLAG=1
    fi
done

if [ $ERROR_FLAG -ne 0 ]; then
    printf "Not all tests passed valgrind.\n"
else
    printf "All tests passed valgrind.\n"
fi

make clean