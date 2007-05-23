AC_PREREQ(2.59)
AC_INIT([libexif command line interface], [0.6.15], [libexif-devel@lists.sourceforge.net], [exif])
AC_CONFIG_SRCDIR([exif/main.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([auto-m4])
AM_INIT_AUTOMAKE([-Wall gnu 1.9 dist-bzip2 check-news])

if test ! -d "$srcdir/m4m"; then
AC_MSG_ERROR([
You are missing the m4m/ directory in your top
$PACKAGE_TARNAME source directory.

You are probably using an ill-maintained CVS tree.
Running

    cd $srcdir
    cvs co m4m

and re-running autogen.sh might help.
])
fi

GP_CHECK_SHELL_ENVIRONMENT
GP_CONFIG_MSG([Build])
GP_CONFIG_MSG([Source code location],[${srcdir}])

AC_PROG_CC
AC_C_CONST
AM_PROG_LIBTOOL
AM_CPPFLAGS="$CPPFLAGS -I\$(top_srcdir) -I\$(top_builddir)"
GP_CONFIG_MSG([Compiler],[${CC}])
AM_PROG_CC_C_O

AC_CHECK_FUNCS([isatty fileno])

GP_PKG_CONFIG

# ---------------------------------------------------------------------------
# i18n support
# ---------------------------------------------------------------------------
GP_GETTEXT_HACK([${PACKAGE}],[Lutz Müller and others])
ALL_LINGUAS="de es fr pl"
AM_GNU_GETTEXT_VERSION([0.14.1])
AM_GNU_GETTEXT([external])
AM_PO_SUBDIRS()
AM_ICONV()
GP_GETTEXT_FLAGS()

# We cannot use [AC_DEFINE_UNQUOTED()] for these definitions, as
# we require make to do insert the proper $(datadir) value
AC_SUBST([localedir],["\$(datadir)/locale"])
AM_CPPFLAGS="$AM_CPPFLAGS -DLOCALEDIR=\\\"${localedir}\\\""

# ---------------------------------------------------------------------------
# locale.h: locale.h provides setlocale. It seems that some systems don't
#           have it (at least if configured --without-nls).
# ---------------------------------------------------------------------------
AC_CHECK_HEADERS([locale.h])
AC_CHECK_HEADER([iconv.h], [
	AC_DEFINE(HAVE_ICONV,1,[whether iconv is available])])


# ---------------------------------------------------------------------------
# libraries needed
# ---------------------------------------------------------------------------
GP_CHECK_LIBRARY([LIBEXIF],[libexif],[>= 0.6.13])


# ---------------------------------------------------------------------------
# Check for misc headers
# ---------------------------------------------------------------------------
AC_CHECK_HEADERS([unistd.h])


# ---------------------------------------------------------------------------
# popt.h: Simplifies handling of command-line options enormously.
# ---------------------------------------------------------------------------
GP_CHECK_POPT([mandatory])dnl


# ---------------------------------------------------------------------------
# Warnings: If we have GCC, be paranoid.
# ---------------------------------------------------------------------------
if test "x$GCC" = "xyes"; then
    AM_CPPFLAGS="$AM_CPPFLAGS -Wall -Wchar-subscripts -Wmissing-declarations -Wmissing-prototypes -Wnested-externs -Wpointer-arith"
    AM_LDFLAGS="$AM_LDFLAGS -g -Wall"
fi

AC_SUBST([AM_CPPFLAGS])dnl
AC_SUBST([AM_LDFLAGS])dnl


# ---------------------------------------------------------------------------
# Output files
# ---------------------------------------------------------------------------
AC_CONFIG_FILES([
  po/Makefile.in
  Makefile
  m4m/Makefile
  exif.spec
  libjpeg/Makefile
  exif/Makefile
])
AC_OUTPUT

GP_CONFIG_OUTPUT()dnl
