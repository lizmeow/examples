#/bin/bash -x

TEST_DIR=tests
USAGE_WORDS=(
    "cat"
    "elizabethhelizabethhelizabethhh"
)
TEST_WORDS=(
    "ccat"
    "dogg"
    "spaghettiii"
    "saghet"
    "cacker"
    "lutter"
)
ERROR_FLAG=0
KEY_PHRASE="All heap blocks were freed -- no leaks are possible"

make clean
make -f Makefile.debug

# Usage tests
r=$(valgrind ./spellcheck $TEST_DIR/corpus1.txt 2>&1)
echo "$r" | grep "$KEY_PHRASE" > /dev/null
if [ $? -ne 0 ]; then
    printf "Usage args test did not pass valgrind.\n"
    ERROR_FLAG=1
fi

# Function tests with single words
for i in "${!USAGE_WORDS[@]}";
do
    r=$(valgrind ./spellcheck $TEST_DIR/corpus1.txt "${USAGE_WORDS[i]}" 2>&1)
    echo "$r" | grep "$KEY_PHRASE" > /dev/null
    if [ $? -ne 0 ]; then
        printf "${USAGE_WORDS[i]} input word did not pass valgrind using corpus $TEST_DIR/corpus1.txt.\n"
        ERROR_FLAG=1
    fi
done

for i in "${!TEST_WORDS[@]}";
do
    r=$(valgrind ./spellcheck $TEST_DIR/corpus2.txt "${TEST_WORDS[i]}" 2>&1)
    echo "$r" | grep "$KEY_PHRASE" > /dev/null
    if [ $? -ne 0 ]; then
        printf "${TEST_WORDS[i]} input word did not pass valgrind using corpus $TEST_DIR/corpus2.txt.\n"
        ERROR_FLAG=1
    fi
done

# Function tests with words contained in a document
r=$(valgrind ./spellcheck tests/corpus1.txt tests/doc1.txt 2>&1)
echo "$r" | grep "$KEY_PHRASE" > /dev/null
if [ $? -ne 0 ]; then
    printf "tests/doc1.txt did not pass valgrind using corpus $TEST_DIR/corpus2.txt.\n"
    ERROR_FLAG=1
fi

if [ $ERROR_FLAG -ne 0 ]; then
    printf "Not all tests passed valgrind.\n"
else
    printf "All tests passed valgrind.\n"
fi

make clean