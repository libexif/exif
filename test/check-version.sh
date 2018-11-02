#!/bin/sh
# Checks that the --version output is sane

. ./check-vars.sh

echo Check version number
$EXIFEXE --version | grep '^[0-9]\+\.[0-9]\+[0-9.]*$' || { echo Invalid version; exit 1; }

echo PASSED
