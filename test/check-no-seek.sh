#!/bin/sh
# Check that operations on files that can't seek are cleanly handled.

. ./check-vars.sh

readonly tmpfifo="check-no-seek-fifo.tmp"
readonly srcimg="$SRCDIR/testdata/no-exif.jpg"
readonly tmpfile="check-no-seek.tmp"

rm -f "${tmpfifo}"
mkfifo "${tmpfifo}" || { echo "Could not create FIFO; skipping test"; exit 0; }

echo Check that write to FIFO succeeds
# Throw away any data written to the FIFO
cat ${tmpfifo} > /dev/null &
$EXIFEXE --create-exif -o "${tmpfifo}" >/dev/null
test $? -eq 0 || { echo Incorrect return code, expected 0; exit 1; }
# Kill the cat and wait for it to exit
kill $!
wait $! 2>/dev/null

echo Check that write to FIFO with early close fails cleanly
# Throw away the first byte of data written to the FIFO then close the FIFO,
# triggering a write error in exif. A FIFO in all tested OSes buffers up to
# 1MiB of data, so the write needs to be larger than that to invoke a write
# failure everywhere. This is done by writing a large image, created by
# appending some dummy JPEG APP15 sections to a small JPEG file.
$EXIFEXE --create-exif "${srcimg}" -o "${tmpfile}"
for n in $(seq 16); do printf '\xff\xef\xff\xff%65531s' filler >>"${tmpfile}"; done
dd if="${tmpfifo}" of=/dev/null bs=1 count=1 2>/dev/null &
$EXIFEXE -r -o "${tmpfifo}" "${tmpfile}" >/dev/null
test $? -eq 1 || { echo Incorrect return code, expected 1; exit 1; }
# Make sure dd is killed and wait for it to exit
kill $!
wait $! 2>/dev/null

echo Check that read of image from unseekable FIFO fails cleanly
# Reading from an image requires its size which can't be determined from a
# FIFO. exif should detect this and cleanly exit. It should really be
# fixed so it doesn't need the size in advance.
# Write the source image to the FIFO twice, since exif reads it twice.
# The sleep is to allow the EOF to be read the first time before it's written
# the second time.
(cat "${srcimg}" >"${tmpfifo}"; sleep 1; cat "${srcimg}" >"${tmpfifo}") &
$EXIFEXE --create-exif -o /dev/null "${tmpfifo}" >/dev/null
test $? -eq 1 || { echo Incorrect return code, expected 1; exit 1; }
# Kill cat and wait for it to exit
kill $!
wait $! 2>/dev/null

echo Check that read of thumbnail from unseekable FIFO fails cleanly
# Same situation as the previous test, but on the thumbnail read path.
(cat "${srcimg}" >"${tmpfifo}"; sleep 1; cat "${srcimg}" >"${tmpfifo}") &
$EXIFEXE --create-exif --insert-thumbnail="${tmpfifo}" -o /dev/null "${srcimg}"
test $? -eq 1 || { echo Incorrect return code, expected 1; exit 1; }
# Kill the cat and wait for it to exit
kill $!
wait $! 2>/dev/null

echo PASSED
rm -f "${tmpfifo}" "${tmpfile}"
