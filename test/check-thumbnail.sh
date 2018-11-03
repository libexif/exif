#!/bin/sh
# Test thumbnail operations

. ./check-vars.sh

readonly tmpfile="check-thumbnail-out.tmp"
readonly tmpimg="check-thumbnail-image.jpg"
readonly tmpimg2="check-thumbnail-image2.jpg"
readonly srcimg="$SRCDIR/testdata/no-exif.jpg"

# Run this in the C locale so the messages are known
LANG=C; export LANG
LANGUAGE=C; export LANGUAGE

echo Create EXIF with thumbnail
cp "$srcimg" "$tmpimg"
# Add image to itself as its own thumbnail
$EXIFEXE --create-exif --insert-thumbnail="$tmpimg" --output="$tmpimg" "$tmpimg" > "$tmpfile" 2>&1
test $? -eq 0 || { echo Incorrect return code; exit 1; }
$DIFFEXE - "$tmpfile" <<EOF
Wrote file 'check-thumbnail-image.jpg'.
EOF
test $? -eq 0 || exit 1

echo Check that thumbnail and tags were added
# Strip off date & time
$EXIFEXE --no-fixup "$tmpimg" 2>&1 | sed -e "/Date and Time/s/|.*$/|/" > "$tmpfile"
$DIFFEXE - "$tmpfile" <<EOF
EXIF tags in 'check-thumbnail-image.jpg' ('Motorola' byte order):
--------------------+----------------------------------------------------------
Tag                 |Value
--------------------+----------------------------------------------------------
X-Resolution        |72
Y-Resolution        |72
Resolution Unit     |Inch
Date and Time       |
YCbCr Positioning   |Centered
Compression         |Internal error (unknown value 0)
X-Resolution        |72
Y-Resolution        |72
Resolution Unit     |Inch
Exif Version        |Exif Version 2.1
Components Configura|Y Cb Cr -
FlashPixVersion     |FlashPix Version 1.0
Color Space         |Uncalibrated
Pixel X Dimension   |0
Pixel Y Dimension   |0
--------------------+----------------------------------------------------------
EXIF data contains a thumbnail (857 bytes).
EOF
test $? -eq 0 || exit 1

echo Add thumbnail to file that already has a thumbnail
$EXIFEXE --insert-thumbnail="$srcimg" --output="$tmpimg2" "$tmpimg" 2>&1 | sed -e "/Date and Time/s/|.*$/|/" > "$tmpfile"
test $? -eq 0 || { echo Incorrect return code; exit 1; }
$DIFFEXE - "$tmpfile" <<EOF
EXIF tags in 'check-thumbnail-image.jpg' ('Motorola' byte order):
--------------------+----------------------------------------------------------
Tag                 |Value
--------------------+----------------------------------------------------------
X-Resolution        |72
Y-Resolution        |72
Resolution Unit     |Inch
Date and Time       |
YCbCr Positioning   |Centered
Compression         |Internal error (unknown value 0)
X-Resolution        |72
Y-Resolution        |72
Resolution Unit     |Inch
Exif Version        |Exif Version 2.1
Components Configura|Y Cb Cr -
FlashPixVersion     |FlashPix Version 1.0
Color Space         |Uncalibrated
Pixel X Dimension   |0
Pixel Y Dimension   |0
--------------------+----------------------------------------------------------
EXIF data contains a thumbnail (857 bytes).
Wrote file 'check-thumbnail-image2.jpg'.
EOF
test $? -eq 0 || exit 1

echo Extract thumbnail
$EXIFEXE --extract-thumbnail --output="$tmpimg2" "$tmpimg" > "$tmpfile" 2>&1
test $? -eq 0 || { echo Incorrect return code; exit 1; }
$DIFFEXE - "$tmpfile" <<EOF
Wrote file 'check-thumbnail-image2.jpg'.
EOF
test $? -eq 0 || exit 1
cmp "$srcimg" "$tmpimg2"
test $? -eq 0 || { echo Thumbnail was corrupted; exit 1; }

echo Remove thumbnail
$EXIFEXE --remove-thumbnail --output="$tmpimg2" "$tmpimg" 2>&1 | sed -e "/Date and Time/s/|.*$/|/" > "$tmpfile"
test $? -eq 0 || { echo Incorrect return code; exit 1; }
$DIFFEXE - "$tmpfile" <<EOF
EXIF tags in 'check-thumbnail-image.jpg' ('Motorola' byte order):
--------------------+----------------------------------------------------------
Tag                 |Value
--------------------+----------------------------------------------------------
X-Resolution        |72
Y-Resolution        |72
Resolution Unit     |Inch
Date and Time       |
YCbCr Positioning   |Centered
Exif Version        |Exif Version 2.1
Components Configura|Y Cb Cr -
FlashPixVersion     |FlashPix Version 1.0
Color Space         |Uncalibrated
Pixel X Dimension   |0
Pixel Y Dimension   |0
--------------------+----------------------------------------------------------
Wrote file 'check-thumbnail-image2.jpg'.
EOF
test $? -eq 0 || exit 1

echo Check that thumbnail and tags were removed
$EXIFEXE --no-fixup "$tmpimg2" 2>&1 | sed -e "/Date and Time/s/|.*$/|/" > "$tmpfile"
$DIFFEXE - "$tmpfile" <<EOF
EXIF tags in 'check-thumbnail-image2.jpg' ('Motorola' byte order):
--------------------+----------------------------------------------------------
Tag                 |Value
--------------------+----------------------------------------------------------
X-Resolution        |72
Y-Resolution        |72
Resolution Unit     |Inch
Date and Time       |
YCbCr Positioning   |Centered
Exif Version        |Exif Version 2.1
Components Configura|Y Cb Cr -
FlashPixVersion     |FlashPix Version 1.0
Color Space         |Uncalibrated
Pixel X Dimension   |0
Pixel Y Dimension   |0
--------------------+----------------------------------------------------------
EOF
test $? -eq 0 || exit 1

echo Extract thumbnail from file without thumbnail
$EXIFEXE --extract-thumbnail --output="$tmpimg" "$tmpimg2" > "$tmpfile" 2>&1
test $? -eq 1 || { echo Incorrect return code; exit 1; }
$DIFFEXE - "$tmpfile" <<EOF
'check-thumbnail-image2.jpg' does not contain a thumbnail!
EOF
test $? -eq 0 || exit 1

echo Remove thumbnail on file without thumbnail
$EXIFEXE --remove-thumbnail --output="$tmpimg" "$tmpimg2" 2>&1 | sed -e "/Date and Time/s/|.*$/|/" > "$tmpfile"
test $? -eq 0 || { echo Incorrect return code; exit 1; }
$DIFFEXE - "$tmpfile" <<EOF
EXIF tags in 'check-thumbnail-image2.jpg' ('Motorola' byte order):
--------------------+----------------------------------------------------------
Tag                 |Value
--------------------+----------------------------------------------------------
X-Resolution        |72
Y-Resolution        |72
Resolution Unit     |Inch
Date and Time       |
YCbCr Positioning   |Centered
Exif Version        |Exif Version 2.1
Components Configura|Y Cb Cr -
FlashPixVersion     |FlashPix Version 1.0
Color Space         |Uncalibrated
Pixel X Dimension   |0
Pixel Y Dimension   |0
--------------------+----------------------------------------------------------
Wrote file 'check-thumbnail-image.jpg'.
EOF
test $? -eq 0 || exit 1

echo Check adding nonexistent thumbnail
$EXIFEXE --insert-thumbnail="does-not-exist" --output="$tmpimg2" "$tmpimg" > "$tmpfile" 2>&1
test $? -eq 1 || { echo Incorrect return code; exit 1; }
$DIFFEXE - "$tmpfile" <<EOF
Could not open 'does-not-exist' (No such file or directory)!
EOF
test $? -eq 0 || exit 1

echo Create tag on thumbnail IFD with no thumbnail
cp "$srcimg" "$tmpimg"
# First, create a file with an extra tag on IFD 1 and verify that it's there.
$EXIFEXE --create-exif --ifd=1 --tag=XResolution --set-value="99 1" --output="$tmpimg" "$tmpimg" > "$tmpfile" 2>&1
test $? -eq 0 || { echo Incorrect return code; exit 1; }
$EXIFEXE --no-fixup "$tmpimg" 2>&1 | sed -e "/Date and Time/s/|.*$/|/" > "$tmpfile"
$DIFFEXE - "$tmpfile" <<EOF
EXIF tags in 'check-thumbnail-image.jpg' ('Motorola' byte order):
--------------------+----------------------------------------------------------
Tag                 |Value
--------------------+----------------------------------------------------------
X-Resolution        |72
Y-Resolution        |72
Resolution Unit     |Inch
Date and Time       |
YCbCr Positioning   |Centered
X-Resolution        |99
Exif Version        |Exif Version 2.1
Components Configura|Y Cb Cr -
FlashPixVersion     |FlashPix Version 1.0
Color Space         |Uncalibrated
Pixel X Dimension   |0
Pixel Y Dimension   |0
--------------------+----------------------------------------------------------
EOF
test $? -eq 0 || exit 1

echo Check that thumbnail tags are removed with no thumbnail
# The tag on IFD 1 is removed because the file has no thumbnail.
$EXIFEXE --output="$tmpimg" "$tmpimg" | sed -e "/Date and Time/s/|.*$/|/" 2>&1 > "$tmpfile"
test $? -eq 0 || { echo Incorrect return code; exit 1; }
$DIFFEXE - "$tmpfile" <<EOF
EXIF tags in 'check-thumbnail-image.jpg' ('Motorola' byte order):
--------------------+----------------------------------------------------------
Tag                 |Value
--------------------+----------------------------------------------------------
X-Resolution        |72
Y-Resolution        |72
Resolution Unit     |Inch
Date and Time       |
YCbCr Positioning   |Centered
Exif Version        |Exif Version 2.1
Components Configura|Y Cb Cr -
FlashPixVersion     |FlashPix Version 1.0
Color Space         |Uncalibrated
Pixel X Dimension   |0
Pixel Y Dimension   |0
--------------------+----------------------------------------------------------
EOF
test $? -eq 0 || exit 1

# Cleanup
echo PASSED
rm -f "$tmpfile" "$tmpimg" "$tmpimg2"
