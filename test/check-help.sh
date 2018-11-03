#!/bin/sh
# Test that --help output is sane

. ./check-vars.sh

# Run this in the C locale so the messages are known
LANG=C; export LANG
LANGUAGE=C; export LANGUAGE

# If these random help strings are found, the rest are probably also there
echo Check help output
$EXIFEXE --help | grep '^  -c, --create-exif               Create EXIF data if not existing$' >/dev/null
test $? -eq 0 || exit 1

$EXIFEXE -\? | grep '^  -r, --remove-thumbnail          Remove thumbnail$' >/dev/null
test $? -eq 0 || exit 1

$EXIFEXE | grep '^  -r, --remove-thumbnail          Remove thumbnail$' >/dev/null
test $? -eq 0 || exit 1

echo PASSED
