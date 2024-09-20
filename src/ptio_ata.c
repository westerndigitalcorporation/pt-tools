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
#include <dirent.h>

#include "ptio.h"

/*
 * For 28-bits commands:
 *  - The FEATURE, COUNT, DEVICE and COMMAND fields are 1 Byte
 *  - The LBA field is 28-bits, using 4 bytes.
 * Total CDB size: 8 Bytes.
 *
 * For 48-bits commands:
 *  - The FEATURE and COUNT fields are 2 Bytes
 *  - The DEVICE and COMMAND fields are 1 Byte
 *  - The LBA field is 48-bits, using 6 bytes.
 * Total CDB size: 12 Bytes.
 *
 * Note: The ICC and AUXILIARY fields are not supported for now.
 */
#define PTIO_ATA_LBA28_CDBSZ	8
#define PTIO_ATA_LBA48_CDBSZ	12

enum ptio_ata_prot {
	PTIO_ATA_NOD = 0, 	/* Non-data */
	PTIO_ATA_PIN,		/* PIO data IN */
	PTIO_ATA_POU,		/* PIO data OUT */
	PTIO_ATA_DMA,		/* DMA */
	PTIO_ATA_EXD,		/* Execute device diagnostics */
};

struct ptio_ata_cmd {
	uint8_t			opcode;
	bool			(*match)(struct ptio_ata_cmd *,
					 uint8_t *, size_t);
	uint16_t		match_data;
	enum ptio_ata_prot	prot;
	bool			ncq;
	bool			lba_48;
	const char		*name;
};

static bool ptio_ata_match_opcode(struct ptio_ata_cmd *cmd,
				  uint8_t *cdb, size_t cdbsz)
{
	/* For both 28-bits and 48-bits CDBs, the last byte is the opcode. */
	return cdb[cdbsz - 1] == cmd->opcode;
}

static bool ptio_ata_match_feat(struct ptio_ata_cmd *cmd,
				uint8_t *cdb, size_t cdbsz)
{
	uint16_t features;

	if (!ptio_ata_match_opcode(cmd, cdb, cdbsz))
		return false;

	/* Feature field */
	if (cmd->lba_48)
		features = ((uint16_t)cdb[0] << 8) | (uint16_t)cdb[1];
	else
		features = (uint16_t)cdb[0];

	return features == cmd->match_data;
}

static bool ptio_ata_match_feat_f(struct ptio_ata_cmd *cmd,
				  uint8_t *cdb, size_t cdbsz)
{
	uint16_t features;

	if (!ptio_ata_match_opcode(cmd, cdb, cdbsz))
		return false;

	/* Feature field */
	if (cmd->lba_48)
		features = ((uint16_t)cdb[0] << 8) | (uint16_t)cdb[1];
	else
		features = (uint16_t)cdb[0];

	/* Lower 4-bits of feature field */
	return (features & 0x0F) == cmd->match_data;
}

static bool ptio_ata_match_fq(struct ptio_ata_cmd *cmd,
			      uint8_t *cdb, size_t cdbsz)
{
	uint16_t count;

	if (!ptio_ata_match_opcode(cmd, cdb, cdbsz)) {
		printf(" fq no match opcode\n");
		return false;
	}


	/* Count field */
	if (cmd->lba_48)
		count = ((uint16_t)cdb[2] << 8) | (uint16_t)cdb[3];
	else
		count = (uint16_t)cdb[1];
	printf(" count %x match data %x\n", count, cmd->match_data);

	/* Bits 12:8 of count field */
	return ((count >> 8) & 0x0F) == cmd->match_data;
}

static struct ptio_ata_cmd ata_cmd[] =
{
	{ 0xE5, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false, false, "CHECK_POWER_MODE" },
	{ 0x14, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false,  true, "CLEAR_DEVICE_FAULT_EXT" },
	{ 0x51, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false,  true, "CONFIGURE_STREAM" },
	{ 0x06, ptio_ata_match_opcode, 0x00, PTIO_ATA_DMA, false,  true, "DATA_SET_MANAGEMENT" },
	{ 0x07, ptio_ata_match_opcode, 0x00, PTIO_ATA_DMA, false,  true, "DATA_SET_MANAGEMENT_XL" },
	{ 0x92, ptio_ata_match_opcode, 0x00, PTIO_ATA_POU, false, false, "DOWNLOAD_MICROCODE" },
	{ 0x93, ptio_ata_match_opcode, 0x00, PTIO_ATA_DMA, false, false, "DOWNLOAD_MICROCODE_DMA" },
	{ 0x90, ptio_ata_match_opcode, 0x00, PTIO_ATA_EXD, false, false, "EXECUTE_DEVICE_DIAGNOSTIC" },
	{ 0xE7, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false, false, "FLUSH_CACHE" },
	{ 0xEA, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false,  true, "FLUSH_CACHE_EXT" },

	/* Accessible Max Address Configuration */
	{ 0x78, ptio_ata_match_feat, 0x00, PTIO_ATA_NOD, false,  true, "GET NATIVE MAX ADDRESS EXT" },
	{ 0x78, ptio_ata_match_feat, 0x01, PTIO_ATA_NOD, false,  true, "SET_ACCESSIBLE_MAX_ADDRESS_EXT" },
	{ 0x78, ptio_ata_match_feat, 0x02, PTIO_ATA_NOD, false,  true, "FREEZE_ACCESSIBLE_MAX_ADDRESS_EXT" },

	{ 0x12, ptio_ata_match_opcode, 0x00, PTIO_ATA_DMA, false,  true, "GET_PHYSICAL_ELEMENT_STATUS" },
	{ 0xEC, ptio_ata_match_opcode, 0x00, PTIO_ATA_PIN, false, false, "IDENTIFY_DEVICE" },
	{ 0xE3, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false, false, "IDLE" },
	{ 0xE1, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false, false, "IDLE_IMMEDIATE" },
	{ 0x96, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false,  true, "MUTATE_EXT" },

	/* NCQ NON-DATA */
	{ 0x63, ptio_ata_match_feat_f, 0x00, PTIO_ATA_NOD,  true,  true, "ABORT_NCQ_QUEUE" },
	{ 0x63, ptio_ata_match_feat_f, 0x01, PTIO_ATA_NOD,  true,  true, "DEADLINE_HANDLING" },
	{ 0x63, ptio_ata_match_feat_f, 0x02, PTIO_ATA_NOD,  true,  true, "HYBRID_DEMOTE_BY_SIZE" },
	{ 0x63, ptio_ata_match_feat_f, 0x03, PTIO_ATA_NOD,  true,  true, "HYBRID_CHANGE_BY_LBA_RANGE" },
	{ 0x63, ptio_ata_match_feat_f, 0x04, PTIO_ATA_NOD,  true,  true, "HYBRID_CONTROL" },
	{ 0x63, ptio_ata_match_feat_f, 0x05, PTIO_ATA_NOD,  true,  true, "SET_FEATURES" },
	{ 0x63, ptio_ata_match_feat_f, 0x06, PTIO_ATA_NOD,  true,  true, "ZERO_EXT" },
	{ 0x63, ptio_ata_match_feat_f, 0x07, PTIO_ATA_NOD,  true,  true, "ZAC_MANAGEMENT_OUT" },
	{ 0x63, ptio_ata_match_feat_f, 0x08, PTIO_ATA_NOD,  true,  true, "DURABLE_ORDERED_WRITE_NOTIFICATION" },

	{ 0x00, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false, false, "NOP" },
	{ 0xE4, ptio_ata_match_opcode, 0x00, PTIO_ATA_PIN, false, false, "READ_BUFFER" },
	{ 0xE9, ptio_ata_match_opcode, 0x00, PTIO_ATA_DMA, false, false, "READ_BUFFER_DMA" },
	{ 0xC8, ptio_ata_match_opcode, 0x00, PTIO_ATA_DMA, false, false, "READ_DMA" },
	{ 0x25, ptio_ata_match_opcode, 0x00, PTIO_ATA_DMA, false,  true, "READ_DMA_EXT" },
	{ 0x60, ptio_ata_match_opcode, 0x00, PTIO_ATA_DMA,  true,  true, "READ_FPDMA_QUEUED" },
	{ 0x47, ptio_ata_match_opcode, 0x00, PTIO_ATA_DMA, false,  true, "READ_LOG_DMA_EXT" },
	{ 0x2F, ptio_ata_match_opcode, 0x00, PTIO_ATA_PIN, false,  true, "READ_LOG_EXT" },
	{ 0x20, ptio_ata_match_opcode, 0x00, PTIO_ATA_PIN, false, false, "READ_SECTORS" },
	{ 0x24, ptio_ata_match_opcode, 0x00, PTIO_ATA_PIN, false,  true, "READ_SECTORS_EXT" },
	{ 0x2A, ptio_ata_match_opcode, 0x00, PTIO_ATA_DMA, false,  true, "READ_STREAM_DMA_EXT" },
	{ 0x2B, ptio_ata_match_opcode, 0x00, PTIO_ATA_PIN, false,  true, "READ_STREAM_EXT" },
	{ 0x40, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false, false, "READ_VERIFY_SECTORS" },
	{ 0x42, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false,  true, "READ_VERIFY_SECTORS_EXT" },

	/* RECEIVE FPDMA QUEUED */
	{ 0x65, ptio_ata_match_fq, 0x01, PTIO_ATA_DMA,  true,  true, "RECEIVE_FPDMA_QUEUED / READ_LOG_DMA_EXT" },
	{ 0x65, ptio_ata_match_fq, 0x02, PTIO_ATA_DMA,  true,  true, "RECEIVE_FPDMA_QUEUED / ZAC_MANAGEMENT_IN" },

	{ 0x7C, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false,  true, "REMOVE_ELEMENT_AND_TRUNCATE" },
	{ 0x7E, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false,  true, "REMOVE_ELEMENT_AND_MODIFY_ZONES" },
	{ 0x0B, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false,  true, "REQUEST_SENSE_DATA_EXT" },
	{ 0x7D, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false,  true, "RESTORE_ELEMENTS_AND_REBUILD" },

	/* Sanitize Device */
	{ 0xB4, ptio_ata_match_feat, 0x00, PTIO_ATA_NOD, false,  true, "SANITIZE_STATUS_EXT" },
	{ 0xB4, ptio_ata_match_feat, 0x11, PTIO_ATA_NOD, false,  true, "CRYPTO_SCRAMBLE_EXT" },
	{ 0xB4, ptio_ata_match_feat, 0x12, PTIO_ATA_NOD, false,  true, "BLOCK_ERASE_EXT" },
	{ 0xB4, ptio_ata_match_feat, 0x14, PTIO_ATA_NOD, false,  true, "OVERWRITE_EXT" },
	{ 0xB4, ptio_ata_match_feat, 0x20, PTIO_ATA_NOD, false,  true, "SANITIZE_FREEZE_LOCK_EXT" },
	{ 0xB4, ptio_ata_match_feat, 0x40, PTIO_ATA_NOD, false,  true, "SANITIZE_ANTIFREEZE_LOCK_EXT" },

	{ 0xF6, ptio_ata_match_opcode, 0x00, PTIO_ATA_POU, false, false, "SECURITY_DISABLE_PASSWORD" },
	{ 0xF3, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false, false, "SECURITY_ERASE_PREPARE" },
	{ 0xF4, ptio_ata_match_opcode, 0x00, PTIO_ATA_POU, false, false, "SECURITY_ERASE_UNIT" },
	{ 0xF5, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false, false, "SECURITY_FREEZE_LOCK" },
	{ 0xF1, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false, false, "SECURITY_SET_PASSWORD" },
	{ 0xF2, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false, false, "SECURITY_UNLOCK" },

	/* SEND FPDMA QUEUED */
	{ 0x64, ptio_ata_match_fq, 0x00, PTIO_ATA_DMA, true,  true, "SEND_FPDMA_QUEUED / DATA_SET_MANAGEMENT" },
	{ 0x64, ptio_ata_match_fq, 0x01, PTIO_ATA_DMA, true,  true, "SEND_FPDMA_QUEUED / HYBRID_EVICT" },
	{ 0x64, ptio_ata_match_fq, 0x02, PTIO_ATA_DMA, true,  true, "SEND_FPDMA_QUEUED / WRITE_LOG_DMA_EXT" },
	{ 0x64, ptio_ata_match_fq, 0x03, PTIO_ATA_DMA, true,  true, "SEND_FPDMA_QUEUED / ZAC_MANAGEMENT_OUT" },
	{ 0x64, ptio_ata_match_fq, 0x04, PTIO_ATA_DMA, true,  true, "SEND_FPDMA_QUEUED / DATA_SET_MANAGEMENT_XL" },
	{ 0x64, ptio_ata_match_fq, 0x05, PTIO_ATA_DMA, true,  true, "SEND_FPDMA_QUEUED / WRITE_GATHERED_EXT" },

	{ 0x77, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false,  true, "SET_DATE_AND_TIME_EXT" },
	{ 0xEF, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false, false, "SET_FEATURES" },
	{ 0xB2, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false,  true, "SET_SECTOR_CONFIGURATON_EXT" },
	{ 0xE6, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false, false, "SLEEP" },

	/* SMART */
	{ 0xB0, ptio_ata_match_feat, 0xD5, PTIO_ATA_PIN, false, false, "SMART_READ_LOG" },
	{ 0xB0, ptio_ata_match_feat, 0xD6, PTIO_ATA_POU, false, false, "SMART_WRITE_LOG" },
	{ 0xB0, ptio_ata_match_feat, 0xDA, PTIO_ATA_NOD, false, false, "SMART_RETURN_STATUS" },

	{ 0xE2, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false, false, "STANDBY" },
	{ 0xE0, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false, false, "STANDBY_IMMEDIATE" },
	{ 0x5B, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false, false, "TRUSTED_NONDATA" },
	{ 0x5C, ptio_ata_match_opcode, 0x00, PTIO_ATA_PIN, false, false, "TRUSTED_RECEIVE" },
	{ 0x5D, ptio_ata_match_opcode, 0x00, PTIO_ATA_DMA, false, false, "TRUSTED_RECEIVE_DMA" },
	{ 0x5E, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false, false, "TRUSTED_SEND" },
	{ 0x5F, ptio_ata_match_opcode, 0x00, PTIO_ATA_DMA, false, false, "TRUSTED_SEND_DMA" },
	{ 0xE8, ptio_ata_match_opcode, 0x00, PTIO_ATA_POU, false, false, "WRITE_BUFFER" },
	{ 0xEB, ptio_ata_match_opcode, 0x00, PTIO_ATA_DMA, false, false, "WRITE_BUFFER_DMA" },
	{ 0xCA, ptio_ata_match_opcode, 0x00, PTIO_ATA_DMA, false, false, "WRITE_DMA" },
	{ 0x35, ptio_ata_match_opcode, 0x00, PTIO_ATA_DMA, false,  true, "WRITE_DMA_EXT" },
	{ 0x3D, ptio_ata_match_opcode, 0x00, PTIO_ATA_DMA, false,  true, "WRITE_DMA_FUA_EXT" },
	{ 0x61, ptio_ata_match_opcode, 0x00, PTIO_ATA_DMA,  true,  true, "WRITE_FPDMA_QUEUED" },
	{ 0x66, ptio_ata_match_opcode, 0x00, PTIO_ATA_DMA, false,  true, "WRITE_GATHERED_EXT" },
	{ 0x57, ptio_ata_match_opcode, 0x00, PTIO_ATA_DMA, false,  true, "WRITE_LOG_DMA_EXT" },
	{ 0x3F, ptio_ata_match_opcode, 0x00, PTIO_ATA_POU, false,  true, "WRITE_LOG_EXT" },
	{ 0x30, ptio_ata_match_opcode, 0x00, PTIO_ATA_POU, false, false, "WRITE_SECTORS" },
	{ 0x34, ptio_ata_match_opcode, 0x00, PTIO_ATA_POU, false,  true, "WRITE_SECTORS_EXT" },
	{ 0x3A, ptio_ata_match_opcode, 0x00, PTIO_ATA_DMA, false,  true, "WRITE_STREAM_DMA_EXT" },
	{ 0x3B, ptio_ata_match_opcode, 0x00, PTIO_ATA_POU, false,  true, "WRITE_STREAM_EXT" },
	{ 0x45, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false,  true, "WRITE_UNCORRECTABLE_EXT" },
	{ 0x44, ptio_ata_match_opcode, 0x00, PTIO_ATA_NOD, false,  true, "ZERO_EXT" },
	{  },
};

static struct ptio_ata_cmd *ptio_ata_find_cmd(uint8_t *cdb, size_t cdbsz)
{
	struct ptio_ata_cmd *cmd = &ata_cmd[0];

	while (cmd->name) {
		if (cmd->match(cmd, cdb, cdbsz))
			return cmd;
		cmd++;
	}

	return NULL;
}

/*
 * Generate an ATA 16 Passthrough SCSI command for the ATA command.
 */
static int ptio_ata_prepare_scsi_cdb(struct ptio_dev *dev,
				     struct ptio_cmd *cmd,
				     struct ptio_ata_cmd *atacmd,
				     uint8_t *cdb, size_t cdbsz)
{
	uint8_t t_dir, t_length = 0;
	uint8_t prot, extend;
	uint64_t lba;

	/* ATA 16 Passthrough */
	cmd->cdbsz = 16;
	cmd->cdb[0] = 0x85;

	if (atacmd->ncq) {
		prot = 0x0C;

		/* FOr NCQ commands, length is in the feature field */
		if (cmd->bufsz)
			t_length = 0x1;
	} else {
		switch (atacmd->prot) {
		case PTIO_ATA_NOD:
			prot = 0x03;
			break;
		case PTIO_ATA_PIN:
			prot = 0x04;
			break;
		case PTIO_ATA_POU:
			prot = 0x05;
			break;
		case PTIO_ATA_DMA:
			prot = 0x06;
			break;
		case PTIO_ATA_EXD:
			prot = 0x08;
			break;
		default:
			ptio_dev_err(dev, "Invalid protocol for 28-bits command %s\n",
				     atacmd->name);
			return -1;
		}

		/* For non-NCQ commands, length is in the count field */
		if (cmd->bufsz)
			t_length = 0x2;
	}

	if (atacmd->lba_48)
		extend = 1;
	else
		extend = 0;

	if (cmd->dxfer == PTIO_DXFER_FROM_DEV)
		t_dir = 1;
	else
		t_dir = 0;

	cmd->cdb[1] = ((prot & 0x0f) << 1) | (extend & 0x01);
	cmd->cdb[2] = ((t_dir & 0x01) << 3) |
		(1 << 2) | /* Number of 512-B blocks to be transferred */
		(t_length & 0x03);

	if (atacmd->lba_48) {
		cmd->cdb[3] = cdb[0]; /* Features 15:8 */
		cmd->cdb[4] = cdb[1]; /* Features 7:0 */

		cmd->cdb[5] = cdb[2]; /* Count 15:8 */
		cmd->cdb[6] = cdb[3]; /* Count 7:0 */

		lba = (uint64_t)cdb[4] << 40 |
			(uint64_t)cdb[5] << 32 |
			(uint64_t)cdb[6] << 24 |
			(uint64_t)cdb[7] << 16 |
			(uint64_t)cdb[8] << 8 |
			(uint64_t)cdb[9];

		cmd->cdb[7] = (lba >> 24) & 0xff; /* LBA 31:24 */
		cmd->cdb[8] = lba & 0xff; /* LBA 7:0 */
		cmd->cdb[9] = (lba >> 32) & 0xff; /* LBA (39:32) */
		cmd->cdb[10] = (lba >> 8) & 0xff; /* LBA (15:8) */
		cmd->cdb[11] = (lba >> 40) & 0xff; /* LBA (47:40) */
		cmd->cdb[12] = (lba >> 16) & 0xff; /* LBA (23:16) */

		cmd->cdb[13] = cdb[10]; /* Device */

		cmd->cdb[14] = cdb[11]; /* Command */

		cmd->cdb[15] = 0; /* Control */

	} else {
		cmd->cdb[4] = cdb[0]; /* Features */

		cmd->cdb[6] = cdb[1]; /* Count */

		lba = ((uint32_t)cdb[2] & 0x0f) << 24 |
			(uint32_t)cdb[3] << 16 |
			(uint32_t)cdb[4] << 8 |
			(uint32_t)cdb[5];

		cmd->cdb[7] = (lba >> 24) & 0x0f; /* LBA 27:24 */
		cmd->cdb[8] = lba & 0xff; /* LBA 7:0 */
		cmd->cdb[10] = (lba >> 8) & 0xff; /* LBA (15:8) */
		cmd->cdb[12] = (lba >> 16) & 0xff; /* LBA (23:16) */

		cmd->cdb[13] = cdb[6]; /* Device */

		cmd->cdb[14] = cdb[7]; /* Command */
	}

	cmd->cdb[15] = 0; /* Control */

	return 0;
}

/*
 * Convert an ATA command into a SCSI passthrough command CDB.
 */
int ptio_ata_prepare_cdb(struct ptio_dev *dev, struct ptio_cmd *cmd,
			 uint8_t *cdb, size_t cdbsz)
{
	struct ptio_ata_cmd *atacmd;

	/* Check the CDB size */
	if (cdbsz != PTIO_ATA_LBA28_CDBSZ && cdbsz != PTIO_ATA_LBA48_CDBSZ) {
		ptio_dev_err(dev, "Invalid ATA CDB size %zu\n", cdbsz);
		return -1;
	}

	/* Find a matching command for the CDB and re-check its size */
	atacmd = ptio_ata_find_cmd(cdb, cdbsz);
	if (!atacmd) {
		ptio_dev_err(dev, "Unknown ATA command\n");
		return -1;
	}

	if (ptio_verbose(dev))
		printf("ATA Command: %s\n", atacmd->name);

	if (!atacmd->lba_48) {
		if (cdbsz != PTIO_ATA_LBA28_CDBSZ) {
			ptio_dev_err(dev,
				     "%s is a 28-bits commands: CDB must be %d B\n",
				     atacmd->name, PTIO_ATA_LBA28_CDBSZ);
			return -1;
		}
	} else if (cdbsz != PTIO_ATA_LBA48_CDBSZ) {
		ptio_dev_err(dev,
			     "%s is a 48-bits commands: CDB must be %d B\n",
			     atacmd->name, PTIO_ATA_LBA48_CDBSZ);
		return -1;
	}

	return ptio_ata_prepare_scsi_cdb(dev, cmd, atacmd, cdb, cdbsz);
}

/*
 * Read a log page.
 */
static int ptio_ata_read_log(struct ptio_dev *dev, uint8_t log,
			    uint16_t page, bool initialize,
			    struct ptio_cmd *cmd, uint8_t *buf, size_t bufsz)
{
	uint8_t cdb[16] = {};

	/*
	 * READ LOG DMA EXT in ATA 16 passthrough command.
	 * +=============================================================+
	 * |  Bit|  7  |  6  |   5   |   4   |   3   |   2   |  1  |  0  |
	 * |Byte |     |     |       |       |       |       |     |     |
	 * |=====+===================+===================================|
	 * | 0   |              Operation Code (85h)                     |
	 * |-----+-------------------------------------------------------|
	 * | 1   |  Multiple count   |            Protocol         | ext |
	 * |-----+-------------------------------------------------------|
	 * | 2   |  off_line |ck_cond| t_type| t_dir |byt_blk| t_length  |
	 * |-----+-------------------------------------------------------|
	 * | 3   |                 features (15:8)                       |
	 * |-----+-------------------------------------------------------|
	 * | 4   |                 features (7:0)                        |
	 * |-----+-------------------------------------------------------|
	 * | 5   |                 count (15:8)                          |
	 * |-----+-------------------------------------------------------|
	 * | 6   |                 count (7:0)                           |
	 * |-----+-------------------------------------------------------|
	 * | 7   |                 LBA (31:24 (15:8 if ext == 0)         |
	 * |-----+-------------------------------------------------------|
	 * | 8   |                 LBA (7:0)                             |
	 * |-----+-------------------------------------------------------|
	 * | 9   |                 LBA (39:32)                           |
	 * |-----+-------------------------------------------------------|
	 * | 10  |                 LBA (15:8)                            |
	 * |-----+-------------------------------------------------------|
	 * | 11  |                 LBA (47:40)                           |
	 * |-----+-------------------------------------------------------|
	 * | 12  |                 LBA (23:16)                           |
	 * |-----+-------------------------------------------------------|
	 * | 13  |                 Device                                |
	 * |-----+-------------------------------------------------------|
	 * | 14  |                 Command (0x47)                        |
	 * |-----+-------------------------------------------------------|
	 * | 15  |                 Control                               |
	 * +=============================================================+
	 */
	cdb[0] = 0x85; /* ATA 16 */
	cdb[1] = (0x6 << 1) | 0x01; /* DMA protocol, ext=1 */
	/* off_line=0, ck_cond=0, t_type=0, t_dir=1, byt_blk=1, t_length=10 */
	cdb[2] = 0x0e;
	if (initialize)
		cdb[4] |= 0x1;
	ptio_set_be16(&cdb[5], bufsz / 512);
	cdb[8] = log;
	ptio_set_be16(&cdb[9], page);
	cdb[14] = 0x47; /* READ LOG DMA EXT */

	/* Execute the command */
	return ptio_exec_cmd(dev, cmd, cdb, 16, PTIO_CDB_SCSI,
			     buf, bufsz, PTIO_DXFER_FROM_DEV);
}

/*
 * Return the number of pages for @log, if it is supported, 0, if @log
 * is not supported, and a negative error code in case of error.
 */
int ptio_ata_log_nr_pages(struct ptio_dev *dev, uint8_t log)
{
	uint8_t buf[512] = {};
	struct ptio_cmd cmd;
	int ret;

	/* Read general purpose log directory */
	ret = ptio_ata_read_log(dev, 0x00, 0x00, false, &cmd, buf, 512);
	if (ret) {
		ptio_dev_err(dev,
			    "Read general purpose log directory failed\n");
		return ret;
	}

	return ptio_get_le16(&buf[log * 2]);
}

static int ptio_ata_get_acs_ver(struct ptio_dev *dev)
{
	uint8_t buf[512] = {};
	struct ptio_cmd cmd;
	int major_ver_num, i, ret;

	ret = ptio_ata_read_log(dev, 0x30, 0x01, false, &cmd, buf, 512);
	if (ret) {
		ptio_dev_err(dev,
			    "Read identify device data log page failed\n");
		return ret;
	}

	/* Get the ACS version supported */
	major_ver_num = ptio_get_le16(&buf[80 * 2]);
	for (i = 8; i < 14; i++) {
		if (major_ver_num & (1 << i))
			dev->acs_ver = i + 1 - 8;
	}

	if (dev->acs_ver < 1) {
		ptio_dev_err(dev, "Invalid major version number\n");
		return -1;
	}

	if (dev->acs_ver > 6) {
		ptio_dev_err(dev, "Unknown ACS major version number 0x%02x\n",
			    major_ver_num);
		return -1;
	}

	return 0;
}

static const char *acs_ver_name[] =
{
	NULL,		/* 0 */
	"ATA8-ACS",	/* 1 */
	"ACS-2",	/* 2 */
	"ACS-3",	/* 3 */
	"ACS-4",	/* 4 */
	"ACS-5",	/* 5 */
	"ACS-6",	/* 6 */
};

const char *ptio_ata_acs_ver(struct ptio_dev *dev)
{
	return acs_ver_name[dev->acs_ver];
}

/*
 * Get ATA information.
 */
int ptio_ata_get_information(struct ptio_dev *dev)
{
        //uint8_t buf[PTIO_SCSI_VPD_PAGE_89_LEN];
        uint8_t buf[512];
        int ret;

        if (!ptio_dev_is_ata(dev)) {
                ptio_dev_err(dev, "Not an ATA device\n");
                return -1;
        }

        /* This is an ATA device: Get SAT information */
        ret = ptio_scsi_vpd_inquiry(dev, 0x89, buf, PTIO_SCSI_VPD_PAGE_89_LEN);
        if (ret != 0) {
                ptio_dev_err(dev, "Get VPD page 0x89 failed\n");
                return ret;
        }

        if (buf[1] != 0x89) {
                ptio_dev_err(dev, "Invalid page code 0x%02x for VPD page 0x00\n",
                            (int)buf[1]);
                return -1;
        }

        ptio_get_str(dev->sat_vendor, &buf[8], PTIO_SAT_VENDOR_LEN - 1);
        ptio_get_str(dev->sat_product, &buf[16], PTIO_SAT_PRODUCT_LEN - 1);
        ptio_get_str(dev->sat_rev, &buf[32], PTIO_SAT_REV_LEN - 1);

        return ptio_ata_get_acs_ver(dev);
}

/*
 * Force device revalidation.
 */
int ptio_ata_revalidate(struct ptio_dev *dev)
{
	const char *scan = "- - -";
	char path[PATH_MAX];
	char host[32], *h;
	struct dirent *dirent;
	FILE *f = NULL;
	int ret = 0;
	DIR *d;

	sprintf(path, "/sys/block/%s/device/scsi_device", dev->name);
	d = opendir(path);
	if (!d) {
		ptio_dev_err(dev, "Open %s failed\n", path);
		return -1;
	}

	while ((dirent = readdir(d))) {
		if (dirent->d_name[0] != '.')
			break;
	}
	if (!dirent) {
		ptio_dev_err(dev, "Read %s failed\n", path);
		goto close;
	}

	strcpy(host, dirent->d_name);
	h = strchr(host, ':');
	if (!h) {
		ptio_dev_err(dev, "Parse %s entry failed\n", path);
		goto close;
	}
	*h = '\0';

	sprintf(path, "/sys/class/scsi_host/host%s/scan", host);

	f = fopen(path, "w");
	if (!f) {
		ptio_dev_err(dev, "Open %s failed\n", path);
		goto close;
	}

	if (!fwrite(scan, strlen(scan), 1, f)) {
		ptio_dev_err(dev, "Write %s failed\n", path);
		ret = -1;
	}

	fclose(f);

close:
	closedir(d);

	return ret;
}

