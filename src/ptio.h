// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024 Western Digital Corporation or its affiliates.
 *
 * Authors: Damien Le Moal (damien.lemoal@wdc.com)
 */
#ifndef PTIO_H
#define PTIO_H

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <scsi/scsi.h>
#include <scsi/sg.h>

/*
 * ATA sector size
 */
#define ATA_SECT_SHIFT	9
#define ATA_SECT_SIZE	(1 << ATA_SECT_SHIFT)

/*
 * Device flags.
 */
#define PTIO_VERBOSE			(1 << 0)
#define PTIO_ATA			(1 << 1)
#define PTIO_INFO			(1 << 2)
#define PTIO_REVALIDATE			(1 << 3)

/*
 * CDB types.
 */
enum ptio_cdb_type {
	PTIO_CDB_NONE = 0,
	PTIO_CDB_SCSI,
	PTIO_CDB_ATA,
};

/*
 * Command data transfer direction.
 */
enum ptio_dxfer {
	PTIO_DXFER_NONE = 0,
	PTIO_DXFER_FROM_DEV,
	PTIO_DXFER_TO_DEV,
};

#define PTIO_VENDOR_LEN	9
#define PTIO_ID_LEN	17
#define PTIO_REV_LEN	5

#define PTIO_SAT_VENDOR_LEN	9
#define PTIO_SAT_PRODUCT_LEN	17
#define PTIO_SAT_REV_LEN	5

#define PTIO_SENSE_MAX_LENGTH	64
#define PTIO_CDB_MAX_SIZE	32

struct ptio_dev {
	/* Device file path and basename */
	char			*path;
	char			*name;

	/* Device file descriptor */
	int			fd;

	/* Device info */
	unsigned int		flags;

	unsigned int		acs_ver;

	char			vendor[PTIO_VENDOR_LEN];
	char			id[PTIO_ID_LEN];
	char			rev[PTIO_REV_LEN];
	char			sat_vendor[PTIO_SAT_VENDOR_LEN];
	char			sat_product[PTIO_SAT_PRODUCT_LEN];
	char			sat_rev[PTIO_SAT_REV_LEN];

	size_t			logical_block_size;
	size_t			physical_block_size;
	unsigned long long	capacity;
};

struct ptio_cmd {
	uint8_t			cdb[PTIO_CDB_MAX_SIZE];
	size_t			cdbsz;
	enum ptio_cdb_type	cdbtype;

	uint8_t			*buf;
	size_t			bufsz;
	enum ptio_dxfer		dxfer;

	sg_io_hdr_t		io_hdr;

	uint8_t			sense_buf[PTIO_SENSE_MAX_LENGTH];
	uint8_t			sense_key;
	uint16_t		asc_ascq;
};

/* In ptio_sense.c */
int ptio_get_sense(struct ptio_dev *dev, struct ptio_cmd *cmd);

/* In ptio_dev.c */
int ptio_open_dev(struct ptio_dev *dev, mode_t mode);
void ptio_close_dev(struct ptio_dev *dev);
int ptio_revalidate_dev(struct ptio_dev *dev);
int ptio_get_dev_information(struct ptio_dev *dev);

int ptio_parse_cdb(char *cdb_str, uint8_t *cdb);

uint8_t *ptio_alloc_buf(size_t bufsz);
uint8_t *ptio_read_buf(char *path, size_t *bufsz);
int ptio_write_buf(char *path, uint8_t *buf, size_t bufsz);
void ptio_print_buf(uint8_t *buf, size_t bufsz);

int ptio_exec_cmd(struct ptio_dev *dev, struct ptio_cmd *cmd,
		  uint8_t *cdb, size_t cdbsz, enum ptio_cdb_type cdb_type,
		  uint8_t *buf, size_t bufsz, enum ptio_dxfer dxfer);

void ptio_get_str(char *dst, uint8_t *buf, int len);
void ptio_set_be16(uint8_t *buf, uint16_t val);
void ptio_set_be32(uint8_t *buf, uint32_t val);
void ptio_set_be64(uint8_t *buf, uint64_t val);
void ptio_set_le16(uint8_t *buf, uint16_t val);
void ptio_set_le32(uint8_t *buf, uint32_t val);
void ptio_set_le64(uint8_t *buf, uint64_t val);
uint16_t ptio_get_be16(uint8_t *buf);
uint32_t ptio_get_be32(uint8_t *buf);
uint64_t ptio_get_be64(uint8_t *buf);
uint16_t ptio_get_le16(uint8_t *buf);
uint32_t ptio_get_le32(uint8_t *buf);
uint64_t ptio_get_le64(uint8_t *buf);

unsigned long ptio_sysfs_get_ulong_attr(struct ptio_dev *dev,
				       const char *format, ...);
int ptio_sysfs_set_attr(struct ptio_dev *dev, const char *val,
		       const char *format, ...);

/* In ptio_ata.c */
int ptio_ata_prepare_cdb(struct ptio_dev *dev, struct ptio_cmd *cmd,
			 uint8_t *cdb, size_t cdbsz);
int ptio_ata_get_information(struct ptio_dev *dev);
const char *ptio_ata_acs_ver(struct ptio_dev *dev);
int ptio_ata_revalidate(struct ptio_dev *dev);

/* In ptio_scsi.c */
#define PTIO_SCSI_VPD_PAGE_00_LEN	32
#define PTIO_SCSI_VPD_PAGE_89_LEN	0x238

int ptio_scsi_prepare_cdb(struct ptio_dev *dev, struct ptio_cmd *cmd,
			  uint8_t *cdb, size_t cdbsz);
int ptio_scsi_vpd_inquiry(struct ptio_dev *dev, uint8_t page,
                          uint8_t *buf, size_t bufsz);
int ptio_scsi_get_information(struct ptio_dev *dev);
int ptio_scsi_revalidate(struct ptio_dev *dev);

static inline bool ptio_dev_is_ata(struct ptio_dev *dev)
{
	return dev->flags & PTIO_ATA;
}

static inline bool ptio_verbose(struct ptio_dev *dev)
{
	return dev->flags & PTIO_VERBOSE;
}

#define ptio_dev_info(dev,format,args...)		\
	printf("%s: " format, (dev)->name, ##args)

#define ptio_dev_err(dev,format,args...)			\
	fprintf(stderr, "[ERROR] %s: " format, (dev)->name, ##args)

#endif /* PTIO_H */
