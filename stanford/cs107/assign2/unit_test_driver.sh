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

make

# Usage tests
./spellcheck $TEST_DIR/corpus1.txt > $TEST_DIR/usage_args.out 2>&1
diff $TEST_DIR/usage_args.ref $TEST_DIR/usage_args.out
if [ $? -ne 0 ]; then
    printf "Usage args test did not pass.\n"
    ERROR_FLAG=1
fi

# Function tests with single words
for i in "${!USAGE_WORDS[@]}";
do
    ./spellcheck $TEST_DIR/corpus1.txt "${USAGE_WORDS[i]}" > $TEST_DIR/usage$i.out 2>&1
    diff $TEST_DIR/usage$i.ref $TEST_DIR/usage$i.out
    if [ $? -ne 0 ]; then
        printf "${USAGE_WORDS[i]} input word did not pass using corpus $TEST_DIR/corpus1.txt.\n"
        ERROR_FLAG=1
    fi
done

for i in "${!TEST_WORDS[@]}";
do
    ./spellcheck $TEST_DIR/corpus2.txt "${TEST_WORDS[i]}" > $TEST_DIR/func$i.out 2>&1
    diff $TEST_DIR/func$i.ref $TEST_DIR/func$i.out
    if [ $? -ne 0 ]; then
        printf "${TEST_WORDS[i]} input word did not pass using corpus $TEST_DIR/corpus2.txt.\n"
        ERROR_FLAG=1
    fi
done

# Function tests with words contained in a document
./spellcheck tests/corpus2.txt tests/doc1.txt > tests/func_doc1.out 2>&1
diff $TEST_DIR/func_doc1.ref $TEST_DIR/func_doc1.out
if [ $? -ne 0 ]; then
    printf "tests/doc1.txt did not pass using corpus $TEST_DIR/corpus2.txt.\n"
    ERROR_FLAG=1
fi

if [ $ERROR_FLAG -ne 0 ]; then
    printf "Not all tests passed.\n"
else
    printf "All tests passed.\n"
fi