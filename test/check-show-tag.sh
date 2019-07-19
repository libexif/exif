#!/bin/sh
# Test output when showing tags

. ./check-vars.sh

readonly tmpfile="check-show-tag-out.tmp"
readonly tmpimg="check-show-tag-image.jpg"
readonly srcimg="$SRCDIR/testdata/no-exif.jpg"

# Run this in the C locale so the messages and decimals are known
LANG=C; export LANG
LANGUAGE=C; export LANGUAGE
LC_NUMERIC=C; export LC_NUMERIC

echo Test default XResolution in IFD 0
$EXIFEXE --create-exif --ifd=0 --tag=XResolution > "$tmpfile" 2>&1
test $? -eq 0 || { echo Incorrect return code; exit 1; }
$DIFFEXE - "$tmpfile" <<EOF
EXIF entry 'X-Resolution' (0x11a, 'XResolution') exists in IFD '0':
Tag: 0x11a ('XResolution')
  Format: 5 ('Rational')
  Components: 1
  Size: 8
  Value: 72
EOF
test $? -eq 0 || exit 1

echo Test default ExifVersion in IFD 1 without specifying IFD
$EXIFEXE --create-exif --tag=ExifVersion > "$tmpfile" 2>&1
test $? -eq 0 || { echo Incorrect return code; exit 1; }
$DIFFEXE - "$tmpfile" <<EOF
EXIF entry 'Exif Version' (0x9000, 'ExifVersion') exists in IFD 'EXIF':
Tag: 0x9000 ('ExifVersion')
  Format: 7 ('Undefined')
  Components: 4
  Size: 4
  Value: Exif Version 2.1
EOF
test $? -eq 0 || exit 1

echo Test valid tag in the wrong IFD
$EXIFEXE --create-exif --tag=ExifVersion --ifd=0 > "$tmpfile" 2>&1
test $? -eq 1 || { echo Incorrect return code; exit 1; }
$DIFFEXE - "$tmpfile" <<EOF
IFD '0' does not contain tag 'ExifVersion'.
EOF
test $? -eq 0 || exit 1

echo Test valid tag that does not exist in any IFD
$EXIFEXE --create-exif --tag=Model > "$tmpfile" 2>&1
test $? -eq 1 || { echo Incorrect return code; exit 1; }
$DIFFEXE - "$tmpfile" <<EOF
'(EXIF)' does not contain tag 'Model'.
EOF
test $? -eq 0 || exit 1

echo Test invalid tag
$EXIFEXE --create-exif --tag=0xbeef > "$tmpfile" 2>&1
test $? -eq 1 || { echo Incorrect return code; exit 1; }
# TODO: exif shouldn't really be writing (null) here
$DIFFEXE - "$tmpfile" <<EOF
'(EXIF)' does not contain tag '(null)'.
EOF
test $? -eq 0 || exit 1

echo Test default XResolution in IFD 0 in machine readable format
$EXIFEXE --create-exif --ifd=0 --tag=XResolution --machine-readable > "$tmpfile" 2>&1
test $? -eq 0 || { echo Incorrect return code; exit 1; }
$DIFFEXE - "$tmpfile" <<EOF
72
EOF
test $? -eq 0 || exit 1

echo Test default ExifVersion in IFD 1 without specifying IFD in machine readable format
$EXIFEXE --create-exif --tag=ExifVersion --machine-readable > "$tmpfile" 2>&1
test $? -eq 0 || { echo Incorrect return code; exit 1; }
$DIFFEXE - "$tmpfile" <<EOF
Exif Version 2.1
EOF
test $? -eq 0 || exit 1

echo Create an image file with one tag in two IFDs for further tests
cp "$srcimg" "$tmpimg"
$EXIFEXE --create-exif --ifd=1 --tag=XResolution --set-value="99 2" --output="$tmpimg" "$tmpimg" 2>&1 > "$tmpfile"
test $? -eq 0 || { echo Incorrect return code; exit 1; }
$DIFFEXE - "$tmpfile" <<EOF
Wrote file 'check-show-tag-image.jpg'.
EOF
test $? -eq 0 || exit 1

echo Test multiple XResolution tags in file
$EXIFEXE --tag=XResolution --no-fixup "$tmpimg" > "$tmpfile" 2>&1
test $? -eq 0 || { echo Incorrect return code; exit 1; }
$DIFFEXE - "$tmpfile" <<EOF
EXIF entry 'X-Resolution' (0x11a, 'XResolution') exists in IFD '0':
Tag: 0x11a ('XResolution')
  Format: 5 ('Rational')
  Components: 1
  Size: 8
  Value: 72
EXIF entry 'X-Resolution' (0x11a, 'XResolution') exists in IFD '1':
Tag: 0x11a ('XResolution')
  Format: 5 ('Rational')
  Components: 1
  Size: 8
  Value: 49.5
EOF
test $? -eq 0 || exit 1

echo Test multiple XResolution tags in file in machine readable format
$EXIFEXE --tag=XResolution --machine-readable --no-fixup "$tmpimg" > "$tmpfile" 2>&1
test $? -eq 0 || { echo Incorrect return code; exit 1; }
$DIFFEXE - "$tmpfile" <<EOF
72
49.5
EOF
test $? -eq 0 || exit 1

# Cleanup
echo PASSED
rm -f "$tmpfile" "$tmpimg"
