#/bin/bash -x

# Author: Elizabeth Howe

KEY_PHRASE="All heap blocks are freed -- no leaks are possible"

make clean
make -f Makefile.debug

r=$(valgrind ./vectortest 2>&1)
echo "$r" | grep "$KEY_PHRASE" > /dev/null
if [ $? -ne 0 ]; then
    printf "vectortest did not pass valgrind.\n"
else
    printf "vectortest passed valgrind.\n"
fi

r=$(valgrind ./hashsettest 2>&1)
echo "$r" | grep "$KEY_PHRASE" > /dev/null
if [ $? -ne 0 ]; then
    printf "hashsettest did not pass valgrind.\n"
else
    printf "hashsettest passed valgrind.\n"
fi

make clean