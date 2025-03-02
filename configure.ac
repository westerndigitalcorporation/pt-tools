# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileCopyrightText: 2024 Western Digital Corporation or its affiliates.

AC_INIT([pt-tools], [2.0.0],
	[damien.lemoal@wdc.com],
	[pt-tools], [https://bitbucket.wdc.com/users/damien.lemoal_wdc.com/repos/pt-tools/browse])

AC_CONFIG_AUX_DIR([build-aux])
AC_CANONICAL_TARGET
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([include/config.h])
AC_PREFIX_DEFAULT(/usr)
AM_INIT_AUTOMAKE([-Wall foreign subdir-objects])
AM_SILENT_RULES([yes])

# Change default CFLAGS from "-g -O2" to "-O2" for regular builds.
AC_ARG_ENABLE(debug,
    [  --enable-debug          Compile with "-g" option],
    [DBGCFLAGS="-g"],
    [DBGCFLAGS="-O2"])
CFLAGS="$CFLAGS $DBGCFLAGS"

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL

AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
m4_pattern_allow([AM_PROG_AR])
LT_INIT

# Package version details: <major>.<minor>.<release>
PACKAGE_VERSION_MAJOR=$(echo $PACKAGE_VERSION | awk -F. '{print $1}')
PACKAGE_VERSION_MINOR=$(echo $PACKAGE_VERSION | awk -F. '{print $2}')
PACKAGE_VERSION_RELEASE=$(echo $PACKAGE_VERSION | awk -F. '{print $3}')

# libtool friendly library version format
LIBPTIO_VERSION_LT=$PACKAGE_VERSION_MAJOR:$PACKAGE_VERSION_MINOR:$PACKAGE_VERSION_RELEASE
AC_SUBST([LIBPTIO_VERSION_LT])

# Checks for header files.
AC_CHECK_HEADER(scsi/scsi.h, [],
		[AC_MSG_ERROR([Couldn't find scsi/scsi.h])])
AC_CHECK_HEADER(scsi/sg.h, [],
		[AC_MSG_ERROR([Couldn't find scsi/sg.h])])
AC_CHECK_HEADER(linux/fs.h, [],
		[AC_MSG_ERROR([Couldn't find linux/fs.h])])

# Checks for rpm package builds
AC_PATH_PROG([RPMBUILD], [rpmbuild], [notfound])
AC_PATH_PROG([RPM], [rpm], [notfound])
AM_CONDITIONAL([BUILD_RPM],
		[test "x$RPMBUILD" != xnotfound && test "x$RPM" != xnotfound])

AC_CONFIG_FILES([
	lib/libptio.pc
	lib/Makefile
	tools/Makefile
	Makefile
])

AC_OUTPUT
