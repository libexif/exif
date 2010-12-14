#!/bin/sh
# Test handling of some basic command-line parameters

. ./check-vars.sh

failed="0"

echo -n "Running \`${EXIFEXE} --help'..."
if ${EXIFEXE} --help > /dev/null; then
    echo " good (return code 0)"
else
    echo " bad (return code != 0)"
    failed="$(expr "$failed" + 1)"
fi

echo -n "Running \`${EXIFEXE} --thisparameterdoesnotexist'..."
if ${EXIFEXE} --thisparameterdoesnotexist > /dev/null; then
    echo " bad (return code 0)"
    failed="$(expr "$failed" + 1)"
else
    echo " good (return code != 0)"
fi

if test "$failed" = "0"; then
    exit 0
else
    echo "FAILED $failed checks."
    exit 1
fi
