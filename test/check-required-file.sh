#!/bin/sh
# Test that operations requiring a file return an error message

. ./check-vars.sh

readonly tmpfile="check-missing-file.tmp"
readonly srcimg="$SRCDIR/testdata/no-exif.jpg"

echo Test that setting a tag requires a file
$EXIFEXE --ifd=0 --tag=Model --set-value="TEST" > "$tmpfile" 2>&1
test $? -eq 1 || { echo Incorrect return code; exit 1; }
$DIFFEXE - "$tmpfile" <<EOF
Specify input file or --create-exif
EOF
test $? -eq 0 || exit 1

echo Test that removing a tag requires a file
$EXIFEXE --remove --ifd=0 --tag=XResolution > "$tmpfile" 2>&1
test $? -eq 1 || { echo Incorrect return code; exit 1; }
$DIFFEXE - "$tmpfile" <<EOF
Specify input file or --create-exif
EOF
test $? -eq 0 || exit 1

echo Test that removing a thumbnail requires a file
$EXIFEXE --remove-thumbnail > "$tmpfile" 2>&1
test $? -eq 1 || { echo Incorrect return code; exit 1; }
$DIFFEXE - "$tmpfile" <<EOF
Specify input file or --create-exif
EOF
test $? -eq 0 || exit 1

echo Test that inserting a thumbnail requires a file
$EXIFEXE --insert-thumbnail="$srcimg" > "$tmpfile" 2>&1
test $? -eq 1 || { echo Incorrect return code; exit 1; }
$DIFFEXE - "$tmpfile" <<EOF
Specify input file or --create-exif
EOF
test $? -eq 0 || exit 1

echo Test that listing tags requires a file
$EXIFEXE --list-tags > "$tmpfile" 2>&1
test $? -eq 1 || { echo Incorrect return code; exit 1; }
$DIFFEXE - "$tmpfile" <<EOF
Specify input file or --create-exif
EOF
test $? -eq 0 || exit 1

echo Test that showing MakerNote requires a file
$EXIFEXE --show-mnote > "$tmpfile" 2>&1
test $? -eq 1 || { echo Incorrect return code; exit 1; }
$DIFFEXE - "$tmpfile" <<EOF
Specify input file or --create-exif
EOF
test $? -eq 0 || exit 1

# Cleanup
echo PASSED
rm -f "$tmpfile"
