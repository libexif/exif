bin_PROGRAMS = exif

exif_SOURCES =			\
	actions.h actions.c	\
	exif-i18n.h exif-i18n.c	\
	main.c			\
	utils.h utils.c
exif_CFLAGS = \
	$(AM_CFLAGS) $(CFLAGS)			\
	$(LIBEXIF_CFLAGS) $(LIBMNOTE_CFLAGS)	\
	$(POPT_CFLAGS)
exif_LDADD = \
	$(AM_LDFLAGS) $(LDFLAGS)		\
	$(top_builddir)/libjpeg/libjpeg.la	\
	$(LIBEXIF_LIBS) $(LIBMNOTE_LIBS)	\
	$(POPT_LIBS)				\
	$(INTLLIBS)
