dnl @synopsis GP_CHECK_LIBRARY([VAR_PREFIX],[libname],[>= version])
dnl
dnl Examples:
dnl    GP_CHECK_LIBRARY([LIBEXIF],[libexif])dnl
dnl    GP_CHECK_LIBRARY([LIBEXIF],[libexif-gtk], [>= 0.3.3])dnl
dnl
AC_DEFUN([_GP_CHECK_LIBRARY],[
AC_ARG_VAR([$1][_CFLAGS], [CFLAGS for compiling with $2])dnl
AC_ARG_VAR([$1][_LIBS],   [libs to add for linking against $2])dnl
AC_MSG_CHECKING([which ][$2][ to use])
if test "x${[$1][_LIBS]}" = "x" && test "x${$1_CFLAGS}" = "x"; then
	AC_MSG_RESULT([autodetect])
	PKG_CHECK_MODULES([$1], [$2][$3], [], [AC_MSG_ERROR([
* Fatal: ${PACKAGE_NAME} requires $2 to build.
])])
elif test "x${[$1][_LIBS]}" != "x" && test "x${[$1][_CFLAGS]}" != "x"; then
	AC_MSG_RESULT([user-defined])
else
	AC_MSG_ERROR([
* Either set [$1][_LIBS] *and* [$1][_CFLAGS] or none at all
* when calling configure for the ${PACKAGE_NAME}.
])
fi
dnl AC_SUBST is done implicitly by AC_ARG_VAR :-)
dnl AC_SUBST([$1][_LIBS])
dnl AC_SUBST([$1][_CFLAGS])
])dnl
dnl
AC_DEFUN([GP_CHECK_LIBRARY], [m4_if([$#], 2,[
# ----------------------------------------------------------------------
# $0([$1],[$2])
# ----------------------------------------------------------------------
_GP_CHECK_LIBRARY([$1],[$2])], [$#], 3,[
# ----------------------------------------------------------------------
# $0([$1],[$2],[$3])
# ----------------------------------------------------------------------
_GP_CHECK_LIBRARY([$1],[$2],[ $3])], [
m4_errprint([Illegal number of arguments ($#) to $0 macro
])m4_exit(1)])dnl
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
