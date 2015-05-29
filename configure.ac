#
# IBM IPR adapter configuration utility
#
# (C) Copyright 2015
# International Business Machines Corporation and others.
# All Rights Reserved. This program and the accompanying
# materials are made available under the terms of the
# Common Public License v1.0 which accompanies this distribution.

AC_PREREQ([2.63])
AC_INIT([iprutils], [2.4.7], [iprdd-devel@lists.sourceforge.net])

AM_INIT_AUTOMAKE([1.9 foreign])
AC_CONFIG_MACRO_DIR([build-aux])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
LT_INIT
AM_PATH_PYTHON([2.6])

ncurses_include=$(ncurses5-config --cflags)
if test -n ${ncurses_include}; then
  CFLAGS="${CFLAGS} ${ncurses_include}"
fi

AC_ARG_ENABLE([build_static],
              [--enable-build-static Generate statically linked binaries],
[case "${enableval}" in
  yes)
     build_static=true
     CFLAGS="-static ${CFLAGS}"
     ;;
  no)
     build_static=false
     ;;
  *)
    AC_MSG_ERROR([bad value ${enableval} for --static])
    ;;
 esac], [build_static=false])
AM_CONDITIONAL([STATIC_BUILD], [test "x${build_static}" = xtrue])

# Check for Libraries.
if test "x${STATIC_BUILD_TRUE}" = "x"; then
 AC_CHECK_LIB(dl, dlopen, [],
	      [AC_ERROR("glibc-static required")])
 AC_CHECK_LIB(tinfo, tgetent, [],
	      [AC_ERROR("libtinfo not found.
	      It should be provided by ncurses")])
fi

AC_CHECK_LIB([ncurses], [curses_version], [],
	     [AC_ERROR("libncurses not found.")])
AC_CHECK_LIB([form], [free_form], [], [AC_ERROR("libform not found.")])
AC_CHECK_LIB([m], [matherr], [], [AC_ERROR("libm not found.")])
AC_CHECK_LIB([menu], [new_menu], [], [AC_ERROR("libmenu not found.")])
AC_CHECK_LIB([panel], [show_panel], [], [AC_ERROR("libpanel not found.")])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h libintl.h locale.h netinet/in.h nl_types.h \
		 paths.h stddef.h stdint.h stdlib.h string.h sys/file.h \
		 sys/ioctl.h sys/mount.h sys/socket.h syslog.h \
		 unistd.h ncurses.h form.h menu.h], [],
		 [AC_ERROR(["Missing headers."])])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([getcwd isascii memset mkdir munmap pow setenv setlocale \
	       socket strcasecmp strchr strncasecmp strrchr strstr \
	       strtol strtoul strtoull])

AC_CONFIG_FILES([Makefile systemd/Makefile init.d/Makefile spec/Makefile])
AC_OUTPUT
