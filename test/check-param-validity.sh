#!/bin/sh
# Test handling of some basic command-line parameters

. ./check-vars.sh

error=0

echo -n "Running \`${EXIFEXE} --help'..."
if ${EXIFEXE} --help > /dev/null; then
    echo " good (return code 0)"
else
    echo " bad (return code != 0)"
    error="$(expr "$error" + 1)"
fi

echo -n "Running \`${EXIFEXE} --thisparameterdoesnotexist'..."
if ${EXIFEXE} --thisparameterdoesnotexist > /dev/null; then
    echo " bad (return code 0)"
    error="$(expr "$error" + 1)"
else
    echo " good (return code != 0)"
fi

if test "$error" = "0"; then
    echo PASSED
    exit 0
else
    echo "FAILED $error checks."
    exit 1
fi
