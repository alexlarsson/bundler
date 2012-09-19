AC_PREREQ([2.63])
AC_INIT([bundler],[0.0.3])
AC_CONFIG_SRCDIR([helper.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE
# Enable silent rules is available
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE([enable])
AC_CONFIG_FILES([Makefile])

AC_SYS_LARGEFILE
AC_PROG_CC
AC_PROG_INSTALL

AC_ARG_WITH(bundle_prefix,
	AS_HELP_STRING([--with-bundle-prefix=DIR],[Prefix used for bundles @<:@/opt/bundle@:>@]),
	[ac_with_bundle_prefix=$withval],
	[ac_with_bundle_prefix="/opt/bundle"])
AC_DEFINE_UNQUOTED(BUNDLE_PREFIX, "$ac_with_bundle_prefix", [The default prefix for bundles])
BUNDLE_PREFIX=$ac_with_bundle_prefix
AC_SUBST(BUNDLE_PREFIX)

if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[[\ \	]]-Wall[[\ \	]]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac
fi

AC_ARG_ENABLE(setuid-install,
		AS_HELP_STRING([--disable-setuid-install],[Don't set setuid flags on binaries during install]),
		[enable_setuid_install=$enableval],
		[enable_setuid_install="yes"])

AM_CONDITIONAL(SETUID_INSTALL, test x"$enable_setuid_install" = "xyes")

AC_OUTPUT
