#!/bin/sh
# Adds tags to an exiting EXIF file. Checks that the tag data is interpreted
# correctly from the command line. Tags representative of as many different
# data types as possible are included. Data inputs with extra spaces,
# plus and minus signs are tested.

. ./check-vars.sh

readonly srcimg="add-tag-src.out.jpg"
readonly dstimg="add-tag.out.jpg"
readonly tmpfile="add-tag.tmp"

# Run this in the C locale so the messages and decimals are known
LANG=C; export LANG
LANGUAGE=C; export LANGUAGE
LC_NUMERIC=C; export LC_NUMERIC

error=0

check_result () {
	s="$?"
	if test "$s" -ne 0; then
		echo " FAILED (${s})."
		error=1
	fi
}

append_image () {
	if [ -e "$dstimg" ] ; then
		mv -f "$dstimg" "$srcimg"
	fi
}

echo Create an empty EXIF tag block
$EXIFEXE --create-exif --no-fixup -o "$srcimg" "$SRCDIR/testdata/no-exif.jpg"
check_result

echo Create SHORT value
$EXIFEXE --no-fixup --ifd=EXIF --tag=FocalLengthIn35mmFilm --set-value='12345' -o "$dstimg" "$srcimg" >/dev/null
check_result
append_image

echo Create SHORT value array
$EXIFEXE --no-fixup --ifd=0 --tag=YCbCrSubSampling --set-value='2 2' -o "$dstimg" "$srcimg" >/dev/null
check_result
append_image

echo 'Create SHORT value (enum)'
$EXIFEXE --no-fixup --ifd=EXIF --tag=SceneCaptureType --set-value='3' -o "$dstimg" "$srcimg" >/dev/null
check_result
append_image

echo 'Create SHORT value (hex tag)'
$EXIFEXE --no-fixup --ifd=EXIF --tag=0xa401 --set-value='2' -o "$dstimg" "$srcimg" >/dev/null
check_result
append_image

echo 'Create SHORT value (decimal tag)'
$EXIFEXE --no-fixup --ifd=EXIF --tag=41986 --set-value=1 -o "$dstimg" "$srcimg" >/dev/null
check_result
append_image

echo 'Create SHORT value (tag title)'
$EXIFEXE --no-fixup --ifd=EXIF --tag='White Balance' --set-value=1 -o "$dstimg" "$srcimg" >/dev/null
check_result
append_image

echo Create LONG value
$EXIFEXE --no-fixup --ifd=EXIF --tag=PixelXDimension --set-value=64 -o "$dstimg" "$srcimg" >/dev/null
check_result
append_image

echo Create RATIONAL value
$EXIFEXE --no-fixup --ifd=0 --tag=XResolution --set-value=' 99  1' -o "$dstimg" "$srcimg" >/dev/null
check_result
append_image

echo Create RATIONAL value array
$EXIFEXE --no-fixup --ifd=0 --tag=WhitePoint --set-value='+9 2   19  3 ' -o "$dstimg" "$srcimg" >/dev/null
check_result
append_image

echo Create SRATIONAL value
$EXIFEXE --no-fixup --ifd=EXIF --tag=ExposureBiasValue --set-value='-3 2' -o "$dstimg" "$srcimg" >/dev/null
check_result
append_image

echo Create UNDEFINED value
$EXIFEXE --no-fixup --ifd=EXIF --tag=SceneType --set-value=1 -o "$dstimg" "$srcimg" >/dev/null
check_result
append_image

echo Create UNDEFINED value array
$EXIFEXE --no-fixup --ifd=EXIF --tag=FlashPixVersion --set-value='49 50 51 52' -o "$dstimg" "$srcimg" >/dev/null
check_result
append_image

echo Create ASCII value
$EXIFEXE --no-fixup --ifd=0 --tag=ImageDescription --set-value='The image description' -o "$dstimg" "$srcimg" >/dev/null
check_result
append_image

echo 'Create ASCII value (User Comment)'
$EXIFEXE --no-fixup --ifd=EXIF --tag=UserComment --set-value='The user comment' -o "$dstimg" "$srcimg" >/dev/null
check_result
append_image

# Count the number of tags
numtags=`$EXIFEXE --no-fixup -m -i "$srcimg" | wc -l`
echo Must be 14 tags: $numtags
test "$numtags" -eq 14
check_result

# The remaining tests are failure mode tests

echo Test invalid RATIONAL--not enough components
$EXIFEXE --no-fixup --ifd=0 --tag=XResolution --set-value=99 -o "$dstimg" "$srcimg" >/dev/null || 
test "$?" -eq 1
check_result
append_image

echo Test invalid RATIONAL--not digits
$EXIFEXE --no-fixup --ifd=0 --tag=YResolution --set-value='9 b' -o "$dstimg" "$srcimg" >/dev/null
test "$?" -eq 1
check_result
append_image

echo Test invalid SHORT--no value
$EXIFEXE --no-fixup --ifd=0 --tag=FocalLengthIn35mmFilm --set-value='' -o "$dstimg" "$srcimg" >/dev/null
test "$?" -eq 1
check_result
append_image

echo Test invalid SHORT--invalid tag
$EXIFEXE --no-fixup --ifd=0 --tag=0xbbbb --set-value=1 -o "$dstimg" "$srcimg" >/dev/null
test "$?" -eq 1
check_result
append_image

echo Test invalid RATIONAL--too many components
# exif treats this as a warning and doesn't report an error code
$EXIFEXE --no-fixup --ifd=0 --tag=XResolution --set-value='12 2 3' -o "$dstimg" "$srcimg" >/dev/null
check_result
append_image

echo Create invalid ASCII--too large
$EXIFEXE --no-fixup --ifd=0 --tag=ImageDescription --set-value="`printf '%66000s' foo`" -o "$dstimg" "$srcimg" >/dev/null
test "$?" -eq 1
check_result
append_image

# Check the resulting EXIF file
$EXIFEXE -m -i "$srcimg" >"$tmpfile"
"$DIFFEXE" - "$tmpfile" <<EOF
0x010e	The image description
0x011a	6.0
0x013e	4.5, 6.3
0x0212	YCbCr4:2:0
0x011b	72
0x0128	Inch
0x9204	-1.50 EV
0x9286	The user comment
0xa000	Unknown FlashPix Version
0xa002	64
0xa301	Directly photographed
0xa401	2
0xa402	Manual exposure
0xa403	Manual white balance
0xa405	12345
0xa406	Night scene
0x9000	Exif Version 2.1
0xa001	Uncalibrated
EOF
check_result

rm -f "$srcimg" "$dstimg" "$tmpfile"

echo Test complete: status $error
exit "$error"
