#!/bin/sh
# Test that --help output is sane

. check-vars.sh

# Run this in the C locale so the messages are known
export LANG=C
export LANGUAGE=C

# If this random help string is found, the rest are probably also there
$EXIFEXE --help | grep '^  -c, --create-exif               Create EXIF data if not existing$' >/dev/null

