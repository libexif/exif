INCLUDES =						\
	-I$(top_srcdir)					\
	-I$(top_srcdir)/intl				\
	-DEXIF_LOCALEDIR=\""$(datadir)/locale"\"	\
	$(LIBEXIF_CFLAGS) $(LIBMNOTE_CFLAGS)		\
	$(POPT_CFLAGS)

bin_PROGRAMS = exif

exif_SOURCES =			\
	actions.h actions.c	\
	exif-i18n.h exif-i18n.c	\
	main.c			\
	utils.h utils.c
exif_LDADD =					\
	../libjpeg/libjpeg.la			\
	$(LIBEXIF_LIBS) $(LIBMNOTE_LIBS)	\
	$(POPT_LIBS)				\
	$(INTLLIBS)
