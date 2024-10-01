# SPDX-License-Identifier: CC0-1.0
#
# Copyright (C) 2024 Western Digital Corporation or its affiliates.

AM_CFLAGS = -O2 -Wall -Wextra -Wno-unused-parameter -D_GNU_SOURCE

bin_PROGRAMS = ptio

CFILES = ptio_sense.c \
	 ptio_dev.c \
	 ptio_scsi.c \
	 ptio_ata.c \
	 ptio.c
HFILES = ptio.h

ptio_SOURCES = ${CFILES} ${HFILES}
