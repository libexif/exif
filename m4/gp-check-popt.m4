dnl @synopsis GP_CHECK_POPT(FLAG)
dnl
dnl FLAG must be one of "mandatory" or "optional"
dnl
dnl
AC_DEFUN([GP_CHECK_POPT],[
#
# check for popt
#
AC_REQUIRE([GP_CONFIG_MSG])dnl

if test "x$1" = "xmandatory"; then
	popt_msg="no (popt is mandatory, so ${PACKAGE} will not compile!)"
elif test "x$1" = "xoptional"; then
	popt_msg="no (optional)"
else
	AC_MSG_ERROR([
Error in your configure.{ac,in}:
GP_CHECK_POPT must be called with a valid argument
])
fi

try_popt=true
have_popt=false
AC_ARG_WITH([popt],
[AS_HELP_STRING([--without-popt],[Do not use popt.h])],
[
	if test "x${withval}" = "xno"; then
		try_popt=false
		popt_msg="no (not requested)"
	fi
])
popt_prefix=$ac_default_prefix
AC_ARG_WITH([popt-prefix],
[AS_HELP_STRING([--with-popt-prefix=PREFIX],[Location of popt])],
[
popt_prefix="$withval"
])
if $try_popt; then
	CPPFLAGS_save="$CPPFLAGS"
	CPPFLAGS="-I$popt_prefix/include $CPPFLAGS"
	AC_CHECK_HEADER([popt.h], [
		AC_CHECK_LIB([popt], [poptResetContext], [
			have_popt="true"
			popt_msg="yes"
			AC_DEFINE([HAVE_POPT],1,[whether we have popt])
			POPT_LIBS="-lpopt"
			POPT_CFLAGS="$CPPFLAGS"
		], [
			LDFLAGS_save="$LDFLAGS"
			LDFLAGS="-L$popt_prefix/lib"
			AC_CHECK_LIB([popt], [poptStuffArgs], [
				have_popt=true
				popt_msg="yes (in '$popt_prefix')"
				AC_DEFINE([HAVE_POPT],1,[whether we have popt])
				POPT_LIBS="-L$popt_prefix/lib -lpopt"
				POPT_CFLAGS="$CPPFLAGS"
			], [popt_pmsg="no (couldn't link)"])
			LDFLAGS="$LDFLAGS_save"
		])
	])
	CPPFLAGS="$CPPFLAGS_save"
fi
AM_CONDITIONAL([HAVE_POPT], [$have_popt])
AC_SUBST([POPT_LIBS])
AC_SUBST([POPT_CFLAGS])
if "$have_popt"; then :; else
	if test "x$1" = "xmandatory"; then
		AC_MSG_ERROR([
*** Fatal: ${PACKAGE_NAME} (${PACKAGE_TARNAME}) requires libpopt (popt)
])
	fi
fi
GP_CONFIG_MSG([Use popt],[${popt_msg}])
])dnl

dnl Please do not remove this:
dnl filetype: 7595380e-eff3-49e5-90ab-e40f1d544639
dnl I use this to find all the different instances of this file which 
dnl are supposed to be synchronized.

dnl Local Variables:
dnl mode: autoconf
dnl End:
