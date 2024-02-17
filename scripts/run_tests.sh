#!/bin/sh

SOURCES=$@

mkdir -p target/$BUILD/.tests

for SOURCE in $SOURCES
do
    BASENAME=`basename -- $SOURCE`
    TEST="target/$BUILD/.tests/${BASENAME%.*}.test"

    HAS_TESTING=`cat $SOURCE | awk -F '\n' '/#pragma +testing/ { print }' | wc -l`

    [ $HAS_TESTING == '0' ] && continue

    $CC $CFLAGS -DTEST=1 -o $TEST $SOURCE $DEPS

    if [ -e $TEST ]
    then
        ./$TEST

        if [ $? == 0 ]; then
            printf '\033[1mTESTS \033[32mPASSING\033[39m FOR %s\033[m\n' "$SOURCE"
        else
            printf '\033[1mTESTS \033[31mFAILING\033[39m FOR %s\033[m\n' "$SOURCE"
        fi
    fi
done

exit 0
