#!/bin/sh

SOURCES=$@

mkdir -p target/$BUILD/.tests

for SOURCE in $SOURCES
do
    BASENAME=$(basename -- $SOURCE)
    TEST="target/$BUILD/.tests/${BASENAME%.*}.test"

    HAS_TESTING=$(cat $SOURCE | grep '#pragma\s\+testing' | wc -l)

    [ $HAS_TESTING == '0' ] && continue

    $CC $CFLAGS -DTEST=1 -o $TEST $SOURCE $DEPS

    if [ -e $TEST ]
    then
        ./$TEST

        if [ $? == 0 ]; then
            printf '\x1b[1mTESTS PASSING FOR %s\x1b[m\n' "$SOURCE"
        else
            printf '\x1b[1mTESTS \x1b[31mFAILING\x1b[39m FOR %s\x1b[m\n' "$SOURCE"
        fi
    fi
done

exit 0
