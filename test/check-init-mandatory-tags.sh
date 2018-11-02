#!/bin/sh
# Ensure that all mandatory tags can be created with --set-value

# TODO: add the other mandatory values for all image types

. ./check-vars.sh

readonly dstimg="init-mandatory.out.jpg"
readonly srcimg="$SRCDIR/testdata/no-exif.jpg"

error=0

check_result () {
	s="$?"
	if test "$s" -ne 0; then
		echo " FAILED (${s})."
		error=1
	fi
}

# IFD 0 mandatory entries

echo Create XResolution
$EXIFEXE --create-exif --no-fixup --ifd=0 --tag=XResolution --set-value=' 99  1' -o "$dstimg" "$srcimg" >/dev/null
check_result

echo Create YResolution
$EXIFEXE --create-exif --no-fixup --ifd=0 --tag=YResolution --set-value='123 2' -o "$dstimg" "$srcimg" >/dev/null
check_result

echo Create ResolutionUnit
$EXIFEXE --create-exif --no-fixup --ifd=0 --tag=ResolutionUnit --set-value='3' -o "$dstimg" "$srcimg" >/dev/null
check_result

echo Create DateTime
$EXIFEXE --create-exif --no-fixup --ifd=0 --tag=DateTime --set-value='2010:01:22 03:44:55' -o "$dstimg" "$srcimg" >/dev/null
check_result

echo Create YCbCrPositioning
$EXIFEXE --create-exif --no-fixup --ifd=0 --tag=YCbCrPositioning --set-value='2' -o "$dstimg" "$srcimg" >/dev/null
check_result

# IFD EXIF mandatory entries

echo Create ExifVersion
$EXIFEXE --create-exif --no-fixup --ifd=EXIF --tag=ExifVersion --set-value='48 50 50 49' -o "$dstimg" "$srcimg" >/dev/null
check_result

echo Create ComponentsConfiguration
$EXIFEXE --create-exif --no-fixup --ifd=EXIF --tag=ComponentsConfiguration --set-value='2 3 1 0' -o "$dstimg" "$srcimg" >/dev/null
check_result

echo Create FlashPixVersion
$EXIFEXE --create-exif --no-fixup --ifd=EXIF --tag=FlashPixVersion --set-value='48 49 48 48' -o "$dstimg" "$srcimg" >/dev/null
check_result

echo Create ColorSpace
$EXIFEXE --create-exif --no-fixup --ifd=EXIF --tag=ColorSpace --set-value='2' -o "$dstimg" "$srcimg" >/dev/null
check_result

echo Create PixelXDimension
$EXIFEXE --create-exif --no-fixup --ifd=EXIF --tag=PixelXDimension --set-value='64' -o "$dstimg" "$srcimg" >/dev/null
check_result

echo Create PixelYDimension
$EXIFEXE --create-exif --no-fixup --ifd=EXIF --tag=PixelYDimension --set-value='32' -o "$dstimg" "$srcimg" >/dev/null
check_result

rm -f "$dstimg"

echo Test complete: status $error
exit "$error"


Here are the default values created by exif for the mandatory tags
for which it is able to create default values.

EXIF tags in '(EXIF)' ('Motorola' byte order):
------+------------------------------------------------------------------------
Tag   |Value
------+------------------------------------------------------------------------
0x011a|72.00
0x011b|72.00
0x0128|Inch
0x0213|Centred
0x0132|2010:09:25 23:58:21
0x9000|Exif Version 2.1
0x9101|Y Cb Cr -
0xa000|FlashPix Version 1.0
0xa001|Uncalibrated
0xa002|0
0xa003|0
------+------------------------------------------------------------------------
EXIF tags in '(EXIF)':                         0      1    EXIF    GPS  Interop
0x011a X-Resolution                            *      -      -      -      -   
0x011b Y-Resolution                            *      -      -      -      -   
0x0128 Resolution Unit                         *      -      -      -      -   
0x0132 Date and Time                           *      -      -      -      -   
0x0213 YCbCr Positioning                       *      -      -      -      -   
0x9000 Exif Version                            -      -      *      -      -   
0x9101 Components Configuration                -      -      *      -      -   
0xa000 FlashPixVersion                         -      -      *      -      -   
0xa001 Colour Space                            -      -      *      -      -   
0xa002 Pixel X Dimension                       -      -      *      -      -   
0xa003 Pixel Y Dimension                       -      -      *      -      -   
