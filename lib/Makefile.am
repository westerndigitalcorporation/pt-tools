# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileCopyrightText: 2024 Western Digital Corporation or its affiliates.

AM_CFLAGS = \
        $(CFLAGS) \
        -Wall -Wextra -Wno-unused-parameter \
        -I$(top_srcdir)/include

pkgconfdir = $(libdir)/pkgconfig
pkgconf_DATA = libptio.pc
pkginclude_HEADERS = ../include/libptio/ptio.h

lib_LTLIBRARIES = libptio.la

EXTRA_DIST = exports

CFILES = ptio_sense.c \
	 ptio_dev.c \
	 ptio_scsi.c \
	 ptio_ata.c
HFILES = ptio.h

libptio_la_DEPENDENCIES = exports
libptio_la_SOURCES = $(CFILES) $(HFILES)
libptio_la_CFLAGS = $(AM_CFLAGS) -fPIC
libptio_la_LDFLAGS = \
        -lpthread \
	-Wl,--version-script,exports \
	-version-number @LIBPTIO_VERSION_LT@
