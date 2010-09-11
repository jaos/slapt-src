AC_INIT([slapt-src], [01], [slapt-get-devel@software.jaos.org])
AM_INIT_AUTOMAKE([-Wall foreign])
AC_CONFIG_SRCDIR([src/])
AC_CONFIG_HEADERS([config.h])
dnl IT_PROG_INTLTOOL([0.35.0])

AC_CANONICAL_HOST

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdio.h stdlib.h string.h unistd.h])
AC_CHECK_HEADER(slapt.h, [], [AC_MSG_ERROR(libslapt headers are missing)])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_LIB(z,gzopen, [LIBS="$LIBS -lz"], [AC_MSG_ERROR(libz is missing)])
AC_CHECK_LIB(curl,curl_global_init, [LIBS="$LIBS `curl-config --libs`"], [AC_MSG_ERROR(curl libs are missing)])
AC_CHECK_LIB(gpgme,gpgme_new, [
    LIBS="`gpgme-config --libs` $LIBS"
    AC_CHECK_LIB(slapt,slapt_add_pkg_source_gpg_key,
      [AC_DEFINE(SLAPT_HAS_GPGME, 1, [libslapt has builtin gpgme support])],
      [AC_MSG_WARN(libslapt does not have gpgme support)])
], [AC_MSG_WARN(gpg support disabled)])
AC_CHECK_LIB(slapt,slapt_disk_space_check, [], [AC_MSG_ERROR(libslapt 0.10.2b or greater is required)])

dnl GETTEXT_PACKAGE=slapt-src
dnl AC_SUBST(GETTEXT_PACKAGE)
dnl AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
])
AC_OUTPUT
