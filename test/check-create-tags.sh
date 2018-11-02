#!/bin/sh
# Tests the creation of new EXIF data.
# Checks that the expected tags are created, and that the --no-fixup option
# creates no tags, just an empty EXIF structure.

. ./check-vars.sh

readonly dstimg="create-tags.out.jpg"

# Abort on any command failure
set -e

echo Create an empty EXIF tag block
$EXIFEXE --create-exif --no-fixup -o "$dstimg" "$SRCDIR/testdata/no-exif.jpg"

# Count the number of tags
numtags=`$EXIFEXE --no-fixup -m -i "$dstimg" | wc -l`

echo Must be 0 tags: $numtags
test $numtags = 0

rm -f "$dstimg"

echo Create a EXIF tag block with mandatory and default tags
$EXIFEXE --create-exif -o "$dstimg" "$SRCDIR/testdata/no-exif.jpg"

# Count the number of tags
numtags=`$EXIFEXE --no-fixup -m -i "$dstimg" | wc -l`

echo Must be 11 tags: $numtags
test $numtags = 11

# Cleanup
echo PASSED
rm -f "$dstimg"
