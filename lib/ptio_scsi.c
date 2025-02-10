// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024 Western Digital Corporation or its affiliates.
 *
 * Authors: Damien Le Moal (damien.lemoal@wdc.com)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>

#include "ptio.h"

/*
 * Prepare the CDB for a SCSI command.
 */
int ptio_scsi_prepare_cdb(struct ptio_dev *dev, struct ptio_cmd *cmd,
			  uint8_t *cdb, size_t cdbsz)
{
	/* Nothing much to do here: use the CDB as-is */
	cmd->cdbsz = cdbsz;
	memcpy(cmd->cdb, cdb, cdbsz);

	return 0;
}

/*
 * Fill the buffer with the result of a VPD page INQUIRY command.
 */
int ptio_scsi_vpd_inquiry(struct ptio_dev *dev, uint8_t page,
			  uint8_t *buf, size_t bufsz)
{
	struct ptio_cmd cmd;
	uint8_t cdb[6];
	int ret;

	/* Get the requested page */
	cdb[0] = 0x12;
	cdb[1] = 0x01;
	cdb[2] = page;
	ptio_set_be16(&cdb[3], bufsz);

	/* Execute the SG_IO command */
	ret = ptio_exec_cmd(dev, &cmd, cdb, 6, PTIO_CDB_SCSI,
			    buf, bufsz, PTIO_DXFER_FROM_DEV, 0);
	if (ret) {
		ptio_dev_err(dev, "Get VPD page 0x%02x failed\n", page);
		return -EIO;
	}

	memcpy(buf, cmd.buf, bufsz);

	return 0;
}

/*
 * Get device information.
 */
int ptio_scsi_get_information(struct ptio_dev *dev)
{
	struct ptio_cmd cmd;
	uint64_t capacity;
	uint32_t lba_size;
	uint8_t cdb[16] = {};
	uint8_t buf[64] = {};
	int ret;

	/* Get device model, vendor and version (INQUIRY) */
	cdb[0] = 0x12; /* INQUIRY */
	ptio_set_be16(&cdb[3], 64);

	ret = ptio_exec_cmd(dev, &cmd, cdb, 16, PTIO_CDB_SCSI,
			    buf, 64, PTIO_DXFER_FROM_DEV, 0);
	if (ret) {
		ptio_dev_err(dev, "INQUIRY failed\n");
		return -1;
	}

	ptio_get_str(dev->vendor, &cmd.buf[8], PTIO_VENDOR_LEN - 1);
	ptio_get_str(dev->id, &cmd.buf[16], PTIO_ID_LEN - 1);
	ptio_get_str(dev->rev, &cmd.buf[32], PTIO_REV_LEN - 1);

	/* Get capacity (READ CAPACITY 16) */
	cdb[0] = 0x9e;
	cdb[1] = 0x10;
	ptio_set_be32(&cdb[10], 32);

	ret = ptio_exec_cmd(dev, &cmd, cdb, 16, PTIO_CDB_SCSI,
			    buf, 32, PTIO_DXFER_FROM_DEV, 0);
	if (ret) {
		ptio_dev_err(dev, "READ CAPACITY failed\n");
		return -1;
	}

	capacity = ptio_get_be64(&cmd.buf[0]) + 1;
	lba_size = ptio_get_be32(&cmd.buf[8]);
	dev->capacity = (capacity * lba_size) >> 9;
	dev->logical_block_size = lba_size;
	dev->physical_block_size =
		lba_size * (1U << (cmd.buf[13] & 0x0f));

	return 0;
}

/*
 * Force device revalidation so that sysfs exposes updated command
 * duration limits.
 */
int ptio_scsi_revalidate(struct ptio_dev *dev)
{
	/* Rescan the device */
	return ptio_sysfs_set_attr(dev, "1",
				   "/sys/block/%s/device/rescan",
				   dev->name);
}

