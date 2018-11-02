#!/bin/sh
# Checks tag description lookup. Only tests a representative sample of
# tags, especially those that test some potential boundary conditions
# of the lookup routines.

. ./check-vars.sh

readonly tmpfile="check-tag-description.tmp"

# Run this in the C locale so the messages are known
LANG=C; export LANG
LANGUAGE=C; export LANGUAGE

# clear out the output file
rm -f "$tmpfile"

# List the tags to test
TESTTAGS_GPS="0"   # first in table
TESTTAGS_GPS=$TESTTAGS_GPS" 1"  # same number as in Interoperability IFD

TESTTAGS_Interoperability="1"   # same number as in GPS IFD

TESTTAGS_0="1"                  # doesn't exist in this IFD
TESTTAGS_0=$TESTTAGS_0" 0x100"  # first in table for this IFD
TESTTAGS_0=$TESTTAGS_0" 0xfe"   # exists in table, but not marked as usable in any IFD
TESTTAGS_0=$TESTTAGS_0" 0x8769" # entry for a sub-IFD
				# This currently prints an empty description,
				# which is really a bug in libexif.

TESTTAGS_0=$TESTTAGS_0" 0xbbbb" # not in table, but between two that are
TESTTAGS_0=$TESTTAGS_0" 0xfffe" # second-largest possible number (not in table)

TESTTAGS_1="0x0201"             # only exists in IFD 1

TESTTAGS_EXIF=$TESTTAGS_EXIF" 0x0201" # only exists in IFD 1, not EXIF IFD
TESTTAGS_EXIF=$TESTTAGS_EXIF" 0xa420" # last in table associated with an IFD
TESTTAGS_EXIF=$TESTTAGS_EXIF" 0xea1c" # last in table (not associated with IFD)

for ifd in GPS Interoperability 0 1 EXIF; do
	TESTTAGS=`eval echo \\$TESTTAGS_${ifd}`
	for tag in $TESTTAGS; do
		echo Testing IFD $ifd tag $tag
		$EXIFEXE --tag=$tag --ifd=$ifd -s >>"$tmpfile"
	done
done

echo Test --machine-readable, using first mandatory tag
$EXIFEXE --tag=0x11a --ifd=0 -m -s >>"$tmpfile"

"$DIFFEXE" - "$tmpfile" <<EOF
Tag 'GPS Tag Version' (0x0000, 'GPSVersionID'): Indicates the version of <GPSInfoIFD>. The version is given as 2.0.0.0. This tag is mandatory when <GPSInfo> tag is present. (Note: The <GPSVersionID> tag is given in bytes, unlike the <ExifVersion> tag. When the version is 2.0.0.0, the tag value is 02000000.H).
Tag 'North or South Latitude' (0x0001, 'GPSLatitudeRef'): Indicates whether the latitude is north or south latitude. The ASCII value 'N' indicates north latitude, and 'S' is south latitude.
Tag 'Interoperability Index' (0x0001, 'InteroperabilityIndex'): Indicates the identification of the Interoperability rule. Use "R98" for stating ExifR98 Rules. Four bytes used including the termination code (NULL). see the separate volume of Recommended Exif Interoperability Rules (ExifR98) for other tags used for ExifR98.
Tag 'Image Width' (0x0100, 'ImageWidth'): The number of columns of image data, equal to the number of pixels per row. In JPEG compressed data a JPEG marker is used instead of this tag.
Tag 'New Subfile Type' (0x00fe, 'NewSubfileType'): A general indication of the kind of data contained in this subfile.
Tag 'JPEG Interchange Format' (0x0201, 'JPEGInterchangeFormat'): The offset to the start byte (SOI) of JPEG compressed thumbnail data. This is not used for primary image JPEG data.
Tag 'Image Unique ID' (0xa420, 'ImageUniqueID'): This tag indicates an identifier assigned uniquely to each image. It is recorded as an ASCII string equivalent to hexadecimal notation and 128-bit fixed length.
Tag 'Padding' (0xea1c, 'Padding'): This tag reserves space that can be reclaimed later when additional metadata are added. New metadata can be written in place by replacing this tag with a smaller data element and using the reclaimed space to store the new or expanded metadata tags.
0x011a	XResolution	X-Resolution	The number of pixels per <ResolutionUnit> in the <ImageWidth> direction. When the image resolution is unknown, 72 [dpi] is designated.
EOF
error="$?"

rm -f "$tmpfile"

echo Test complete: status $error
exit "$error"
