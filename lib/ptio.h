// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024 Western Digital Corporation or its affiliates.
 *
 * Authors: Damien Le Moal (damien.lemoal@wdc.com)
 */
#ifndef PTIO_H
#define PTIO_H

#include "config.h"
#include "libptio/ptio.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/ioctl.h>

int ptio_get_sense(struct ptio_dev *dev, struct ptio_cmd *cmd);

void ptio_get_str(char *dst, uint8_t *buf, int len);

unsigned long ptio_sysfs_get_ulong_attr(struct ptio_dev *dev,
				       const char *format, ...);
int ptio_sysfs_set_attr(struct ptio_dev *dev, const char *val,
		       const char *format, ...);

int ptio_ata_prepare_cdb(struct ptio_dev *dev, struct ptio_cmd *cmd,
			 uint8_t *cdb, size_t cdbsz);
int ptio_ata_get_information(struct ptio_dev *dev);
int ptio_ata_revalidate(struct ptio_dev *dev);

#define PTIO_SCSI_VPD_PAGE_00_LEN	32
#define PTIO_SCSI_VPD_PAGE_89_LEN	0x238

int ptio_scsi_prepare_cdb(struct ptio_dev *dev, struct ptio_cmd *cmd,
			  uint8_t *cdb, size_t cdbsz);
int ptio_scsi_vpd_inquiry(struct ptio_dev *dev, uint8_t page,
                          uint8_t *buf, size_t bufsz);
int ptio_scsi_get_information(struct ptio_dev *dev);
int ptio_scsi_revalidate(struct ptio_dev *dev);

static inline bool ptio_verbose(struct ptio_dev *dev)
{
	return dev->flags & PTIO_VERBOSE;
}

#define ptio_dev_info(dev,format,args...)		\
	printf("PTIO (%s): " format, (dev)->name, ##args)

#define ptio_dev_err(dev,format,args...)		\
	fprintf(stderr, "PTIO (%s): [ERROR]: " format, (dev)->name, ##args)

#define ptio_dev_verbose(dev,format,args...)		\
	if (ptio_verbose(dev))				\
		ptio_dev_info(dev, format, ##args)

#endif /* PTIO_H */
