# The actual "exif" program

bin_PROGRAMS = exif

exif_SOURCES =			\
	actions.h actions.c	\
	exif-i18n.h exif-i18n.c	\
	main.c			\
	utils.h utils.c
exif_CPPFLAGS = \
	$(AM_CPPFLAGS) $(CPPFLAGS)		\
	$(LIBEXIF_CFLAGS)			\
	$(POPT_CFLAGS)
exif_LDADD = \
	$(AM_LDFLAGS) $(LDFLAGS)		\
	$(top_builddir)/libjpeg/libjpeg.la	\
	$(LIBEXIF_LIBS)				\
	$(POPT_LIBS)				\
	$(INTLLIBS)

# And a few very simple tests

check_SCRIPTS = check-param-validity.sh

EXTRA_DIST = check-param-validity.in

TESTS = $(check_SCRIPTS)

.in.sh:
	sed 's|@exif\@|./exif$(EXEEXT)|g' < $< > $@
	chmod +x $@

CLEANFILES = $(check_SCRIPTS)
