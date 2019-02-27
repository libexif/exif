#!/bin/sh
# Check that operations on files that can't seek are cleanly handled.

. ./check-vars.sh

readonly tmpfifo="check-no-seek.tmp"
readonly srcimg="$SRCDIR/testdata/no-exif.jpg"

rm -f "${tmpfifo}"
mkfifo "${tmpfifo}" || { echo "Could not create FIFO; skipping test"; exit 0; }

echo Check that write to FIFO succeeds
# Throw away any data written to the FIFO
cat ${tmpfifo} > /dev/null &
$EXIFEXE --create-exif -o "${tmpfifo}" >/dev/null
test $? -eq 0 || { echo Incorrect return code $? not 0; exit 1; }
# Kill the cat and wait for it to exit
kill $!
wait $!

echo Check that read of image from FIFO fails cleanly
# Reading from an image requires its size which can't be determined from a
# FIFO. exif should detect this and cleanly exit. It should really be
# fixed so it doesn't need the size in advance.
# Write the source image to the FIFO twice, since exif reads it twice.
# The sleep is to allow the EOF to be read the first time before it's written
# the second time.
(cat "${srcimg}" >"${tmpfifo}"; sleep 1; cat "${srcimg}" >"${tmpfifo}") &
$EXIFEXE --create-exif -o /dev/null "${tmpfifo}" >/dev/null
test $? -eq 1 || { echo Incorrect return code $? not 1; exit 1; }
# Kill cat and wait for it to exit
kill $!
wait $! >/dev/null 2>&1

echo Check that read of thumbnail from FIFO fails cleanly
(cat "${srcimg}" >"${tmpfifo}"; sleep 1; cat "${srcimg}" >"${tmpfifo}") &
$EXIFEXE --create-exif --insert-thumbnail="${tmpfifo}" -o /dev/null
test $? -eq 1 || { echo Incorrect return code $? not 1; exit 1; }
# Kill the cat and wait for it to exit
kill $!
wait $!

echo PASSED
rm -f "${tmpfifo}"
