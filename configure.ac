#
# IBM IPR adapter configuration utility
#
# (C) Copyright 2015
# International Business Machines Corporation and others.
# All Rights Reserved. This program and the accompanying
# materials are made available under the terms of the
# Common Public License v1.0 which accompanies this distribution.

AC_PREREQ([2.63])
# For RC releases, version should be set to 2.4.10.0.rc1, 2.4.10.0.rc2, etc.
# For GA releases, version should be set to 2.4.10.1, 2.4.10.2, etc.
AC_INIT([iprutils], [2.4.17.1], [iprdd-devel@lists.sourceforge.net])

AM_INIT_AUTOMAKE([1.9 foreign])
AC_CONFIG_MACRO_DIR([build-aux])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
LT_INIT

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

# --enable-sosreport.
AC_ARG_ENABLE([sosreport],
              [--enable-sosreport Install sosreport plugin],
[case "${enableval}" in
  yes)
     sosreport=true
     ;;
  no)
     sosreport=false
     ;;
  *)
    AC_MSG_ERROR([bad value ${enableval} for --enable-sosreport])
    ;;
 esac], [sosreport=false])
AM_CONDITIONAL([SOSREPORT], [test "x${sosreport}" = xtrue])

# --enable-iprdumpfmt.
AC_ARG_ENABLE([iprdumpfmt],
              [--enable-iprdumpfmt Install iprdumpfmt tool],
[case "${enableval}" in
  yes)
     iprdumpfmt=true
     ;;
  no)
     iprdumpfmt=false
     ;;
  *)
    AC_MSG_ERROR([bad value ${enableval} for --enable-iprdumpfmt])
    ;;
 esac], [iprdumpfmt=false])
AM_CONDITIONAL([IPRDUMPFMT], [test "x${iprdumpfmt}" = xtrue])

# --with-systemd
systemdunitdir='${exec_prefix}/lib/systemd/system/'
udevrulesdir='${exec_prefix}/lib/udev/rules.d/'
AC_ARG_WITH([systemd],
	    [--without-systemd Disable systemd service files.],
[case $withval in
     yes)
        systemd=true;
	;;
     no)
	systemd=false;
	;;
     *)
	systemd=true;
	systemdunitdir="$withval"
	;;
 esac], [systemd=true])
AM_CONDITIONAL([SYSTEMD], [test "x${systemd}" = xtrue])
AC_SUBST(systemdunitdir, ${systemdunitdir})
AC_SUBST(udevrulesdir, ${udevrulesdir})

# --with-initd
initddir="${prefix}/etc/init.d/"
AC_ARG_WITH([initscripts],
	    [--with-initscripts Enable initd files.],
[case $withval in
     yes)
        initd=true;
	;;
     no)
	initd=false;
	;;
     *)
	initd=true;
	initddir="$withval"
	;;
 esac], [initd=false])
AM_CONDITIONAL([INITD], [test "x${initd}" = xtrue])
AC_SUBST(initddir, ${initddir})

## Check for python if building with sosreport.
if test "x${SOSREPORT_TRUE}" = "x"; then
   AM_PATH_PYTHON([2.6])
fi

# Check for Libraries.
if test "x${STATIC_BUILD_TRUE}" = "x"; then
 AC_CHECK_LIB(dl, dlopen, [],
	      [AC_ERROR("glibc-static required")])
 AC_CHECK_LIB(tinfo, tgetent, [],
	      [AC_ERROR("libtinfo not found.
	      It should be provided by ncurses")])
fi

save_LIBS="$LIBS"
AC_CHECK_LIB([ncurses], [curses_version], [],
	     [AC_ERROR("libncurses not found.")])
AC_CHECK_LIB([form], [free_form], [], [AC_ERROR("libform not found.")])
AC_CHECK_LIB([m], [pow], [], [AC_ERROR("libm not found.")])
AC_CHECK_LIB([menu], [new_menu], [], [AC_ERROR("libmenu not found.")])
AC_CHECK_LIB([panel], [show_panel], [], [AC_ERROR("libpanel not found.")])
AC_CHECK_LIB([z], [zlibVersion], [], [AC_ERROR("zlib not found.")])
IPRCONFIG_LIBS="$LIBS"
AC_SUBST([IPRCONFIG_LIBS])
LIBS="$save_LIBS"

ncurses_config=$(which ncurses5-config 2> /dev/null)
if ! test -n ${ncurses_config}; then
  ncurses_config=$(which ncurses6-config 2> /dev/null)
fi

if test -n ${ncurses_config}; then
  ncurses_include=$(${ncurses_config} --cflags)

  if test -n "${ncurses_include}"; then
    CFLAGS="${CFLAGS} ${ncurses_include}"
  fi
fi

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h locale.h netinet/in.h nl_types.h \
		 paths.h stddef.h stdint.h stdlib.h string.h sys/file.h \
		 sys/ioctl.h sys/mount.h sys/socket.h syslog.h \
		 unistd.h ncurses.h form.h menu.h zlib.h], [],
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

AC_CONFIG_FILES([Makefile systemd/Makefile init.d/Makefile spec/Makefile udev/Makefile])
AC_OUTPUT
