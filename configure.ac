AC_INIT([slapt-src],[0.3.7],[woodwardj@jaos.org])
AM_INIT_AUTOMAKE([-Wall foreign])
AC_CONFIG_SRCDIR([src/])
AC_CONFIG_HEADERS([config.h])
IT_PROG_INTLTOOL([0.35.0])

AC_PROG_CC dnl now defaults to C2011
AC_CANONICAL_HOST

case "$host_cpu" in
i[[3456]]86) host_cpu=i386 ;;
arm*) host_cpu=arm ;;
esac

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdio.h stdlib.h string.h unistd.h])
AC_CHECK_HEADER(slapt.h, [], [AC_MSG_ERROR(libslapt headers are missing)])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_LIB(z,gzopen, [LIBS="$LIBS -lz"], [AC_MSG_ERROR(libz is missing)])
AC_CHECK_LIB(m,round, [LIBS="$LIBS -lm"], [AC_MSG_ERROR(libm is missing)])
AC_CHECK_LIB(curl,curl_global_init, [LIBS="$LIBS `curl-config --libs`"], [AC_MSG_ERROR(curl libs are missing)])
AC_SEARCH_LIBS([EVP_md5],[ssl crypto],[],[AC_MSG_ERROR(openssl libs are missing)])
AC_CHECK_LIB(gpgme,gpgme_new, [
    LIBS="`gpgme-config --libs` $LIBS"
    AC_CHECK_LIB(slapt,slapt_add_pkg_source_gpg_key,
      [AC_DEFINE(SLAPT_HAS_GPGME, 1, [libslapt has builtin gpgme support])],
      [AC_MSG_WARN(libslapt does not have gpgme support)])
], [AC_MSG_WARN(gpg support disabled)])
AC_CHECK_LIB(slapt,slapt_vector_t_init, [], [AC_MSG_ERROR(libslapt 0.11.1 or greater is required)])

# options
AC_ARG_WITH([fakeroot], 
  AS_HELP_STRING([--with-fakeroot], [use fakeroot (default is no)]),
  [AC_DEFINE(HAS_FAKEROOT, 1, [has fakeroot])],
)
AC_ARG_WITH([slkbuild], 
  AS_HELP_STRING([--with-slkbuild], [use slkbuild (default is no)]),
  [AC_DEFINE(HAS_SLKBUILD, 1, [has slkbuild])],
)

GETTEXT_PACKAGE=slapt-src
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.17])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  po/Makefile.in
])
AC_OUTPUT
