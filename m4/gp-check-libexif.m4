dnl @synopsis GP_CHECK_LIBEXIF
AC_DEFUN([GP_CHECK_LIBEXIF],[
AC_ARG_VAR([LIBEXIF_LIBS],[libs to add for linking against libexif])
AC_ARG_VAR([LIBEXIF_CFLAGS],[CFLAGS for compiling with libexif])
AC_MSG_CHECKING([which libexif to use])
if test "x$LIBEXIF_LIBS" = "x" && test "x$LIBEXIF_CFLAGS" = "x"; then
	AC_MSG_RESULT([autodetect])
	PKG_CHECK_MODULES([LIBEXIF], [libexif >= 0.6.11], [], [AC_MSG_ERROR([
* Fatal: ${PACKAGE_NAME} requires libexif to build.
])])
elif test "x$LIBEXIF_LIBS" != "x" && test "x$LIBEXIF_CFLAGS" != "x"; then
	AC_MSG_RESULT([user-defined])
else
	AC_MSG_ERROR([
* Either set LIBEXIF_LIBS *and* LIBEXIF_CFLAGS or none at all
* when calling configure for the ${PACKAGE_NAME}.
])
fi
AC_SUBST([LIBEXIF_LIBS])
AC_SUBST([LIBEXIF_CFLAGS])
])dnl
dnl
dnl Please do not remove this:
dnl filetype: 6e60b4f0-acb2-4cd5-8258-42014f92bd2c
dnl I use this to find all the different instances of this file which 
dnl are supposed to be synchronized.
dnl
dnl Local Variables:
dnl mode: autoconf
dnl End:
