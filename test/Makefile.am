# "exif" test scripts
# Only tests that work without image files should be found here.
# More complicated tests should be put into the libexif-testsuite module.

check_SCRIPTS = check-param-validity.sh

EXTRA_DIST = check-param-validity.in

TESTS = $(check_SCRIPTS)

.in.sh:
	sed 's|@exif\@|../exif/exif$(EXEEXT)|g' < $< > $@
	chmod +x $@

CLEANFILES = $(check_SCRIPTS)
