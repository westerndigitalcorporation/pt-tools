# SPDX-License-Identifier: GPL-2.0-or-later
#
# SPDX-FileCopyrightText: 2024 Western Digital Corporation or its affiliates.

SUBDIRS = . $(subdirs)

AM_CFLAGS = \
        $(CFLAGS) \
        -Wall -Wextra -Wno-unused-parameter \
        -I$(top_srcdir)/include

libptio_ldadd = $(top_builddir)/lib/libptio.la

bin_PROGRAMS =
dist_man8_MANS =

include cli/Makefile.am
