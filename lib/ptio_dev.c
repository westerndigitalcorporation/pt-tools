// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024 Western Digital Corporation or its affiliates.
 *
 * Authors: Damien Le Moal (damien.lemoal@wdc.com)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <libgen.h>

#include "ptio.h"

/*
 * Set bytes in a SCSI command cdb or buffer.
 */
void ptio_set_be16(uint8_t *buf, uint16_t val)
{
	uint16_t *v = (uint16_t *)buf;

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	*v = __builtin_bswap16(val);
#else
	*v = val;
#endif
}

void ptio_set_be32(uint8_t *buf, uint32_t val)
{
	uint32_t *v = (uint32_t *)buf;

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	*v = __builtin_bswap32(val);
#else
	*v = val;
#endif
}

void ptio_set_be64(uint8_t *buf, uint64_t val)
{
	uint64_t *v = (uint64_t *)buf;

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	*v = __builtin_bswap64(val);
#else
	*v = val;
#endif
}

void ptio_set_le16(uint8_t *buf, uint16_t val)
{
	uint16_t *v = (uint16_t *)buf;

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	*v = val;
#else
	*v = __builtin_bswap16(val);
#endif
}

void ptio_set_le32(uint8_t *buf, uint32_t val)
{
	uint32_t *v = (uint32_t *)buf;

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	*v = val;
#else
	*v = __builtin_bswap32(val);
#endif
}

void ptio_set_le64(uint8_t *buf, uint64_t val)
{
	uint64_t *v = (uint64_t *)buf;

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	*v = val;
#else
	*v = __builtin_bswap64(val);
#endif
}

/*
 * Get bytes from a SCSI command cdb or buffer.
 */
uint16_t ptio_get_be16(uint8_t *buf)
{
	uint16_t val = *((uint16_t *)buf);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return __builtin_bswap16(val);
#else
	return val;
#endif
}

uint32_t ptio_get_be32(uint8_t *buf)
{
	uint32_t val = *((uint32_t *)buf);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return __builtin_bswap32(val);
#else
	return val;
#endif
}

uint64_t ptio_get_be64(uint8_t *buf)
{
	uint64_t val = *((uint64_t *)buf);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return __builtin_bswap64(val);
#else
	return val;
#endif
}

uint16_t ptio_get_le16(uint8_t *buf)
{
	uint16_t val = *((uint16_t *)buf);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return val;
#else
	return __builtin_bswap16(val);
#endif
}

uint32_t ptio_get_le32(uint8_t *buf)
{
	uint32_t val = *((uint32_t *)buf);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return val;
#else
	return __builtin_bswap32(val);
#endif
}

uint64_t ptio_get_le64(uint8_t *buf)
{
	uint64_t val = *((uint64_t *)buf);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return val;
#else
	return __builtin_bswap64(val);
#endif
}

/*
 * Get a string from a SCSI command output buffer.
 */
void ptio_get_str(char *dst, uint8_t *buf, int len)
{
	char *str = (char *) buf;
	int i;

	for (i = len - 1; i >= 0; i--) {
	       if (isalnum(str[i]))
		       break;
	}

	if (i >= 0)
		memcpy(dst, str, i + 1);
}

/*
 * Parse a cdb string.
 */
int ptio_parse_cdb(char *cdb_str, uint8_t *cdb)
{
	size_t len = strlen(cdb_str);
	size_t cdbsz = 0;
	char *p = cdb_str;
	char *p_end = p + len;
	char *pe;
	long b;

	while (p < p_end) {
		if (cdbsz >= PTIO_CDB_MAX_SIZE) {
			fprintf(stderr, "CDB is too large\n");
			return -1;
		}

		if (isblank(*p)) {
			p++;
			continue;
		}

		if (!isxdigit(*p)) {
			fprintf(stderr, "Invalid character in CDB\n");
			return -1;
		}

		b = strtol(p, &pe, 16);
		if (b > 0xff) {
			fprintf(stderr, "Invalid value in CDB\n");
			return -1;
		}
		p = pe;

		cdb[cdbsz] = b;
		cdbsz++;
	}

	if (!cdbsz) {
		fprintf(stderr, "Empty CDB\n");
		return -1;
	}

	return cdbsz;
}

/*
 * Print a cdb
 */
static void ptio_print_cdb(uint8_t *cdb, size_t cdbsz)
{
	unsigned int l = 0, i;

	printf("  +----------+-------------------------------------------------+\n");
	printf("  |  OFFSET  | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |\n");
	printf("  +----------+-------------------------------------------------+\n");

	while (l < cdbsz) {
		printf("  | %08x |", l);
		for (i = 0; i < 16; i++, l++) {
			if (l < cdbsz)
				printf(" %02x", (unsigned int)cdb[l]);
			else
				printf("   ");
		}
		printf(" |\n");
	}

	printf("  +----------+-------------------------------------------------+\n");
}

/*
 * Print a buffer
 */
void ptio_print_buf(uint8_t *buf, size_t bufsz)
{
	unsigned int l = 0, i;

	printf("  +----------+-------------------------------------------------+\n");
	printf("  |  OFFSET  | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |\n");
	printf("  +----------+-------------------------------------------------+\n");

	while (l < bufsz) {
		printf("  | %08x |", l);
		for (i = 0; i < 16; i++, l++) {
			if (l < bufsz)
				printf(" %02x", (unsigned int)buf[l]);
			else
				printf("   ");
		}
		printf(" |\n");
	}

	printf("  +----------+-------------------------------------------------+\n");
}

#define ptio_cmd_driver_status(cmd)	((cmd)->io_hdr.driver_status & \
					 PTIO_DRIVER_STATUS_MASK)
#define ptio_cmd_driver_flags(cmd)	((cmd)->io_hdr.driver_status &  \
					 PTIO_DRIVER_FLAGS_MASK)

int ptio_exec_cmd(struct ptio_dev *dev, struct ptio_cmd *cmd,
		 uint8_t *cdb, size_t cdbsz, enum ptio_cdb_type cdb_type,
		 uint8_t *buf, size_t bufsz, enum ptio_dxfer dxfer)
{
	int ret, sg_dxfer;

	assert(cdbsz <= PTIO_CDB_MAX_SIZE);

	memset(cmd, 0, sizeof(struct ptio_cmd));

	cmd->dxfer = dxfer;
	switch (dxfer) {
	case PTIO_DXFER_NONE:
		sg_dxfer = SG_DXFER_NONE;
		break;
	case PTIO_DXFER_FROM_DEV:
		cmd->buf = buf;
		cmd->bufsz = bufsz;
		sg_dxfer = SG_DXFER_FROM_DEV;
		break;
	case PTIO_DXFER_TO_DEV:
		cmd->buf = buf;
		cmd->bufsz = bufsz;
		sg_dxfer = SG_DXFER_TO_DEV;
		break;
	default:
		ptio_dev_err(dev, "Invalid data transfer direction\n");
		return -1;
	}

	cmd->cdbtype = cdb_type;
	switch (cdb_type) {
	case PTIO_CDB_SCSI:
		ret = ptio_scsi_prepare_cdb(dev, cmd, cdb, cdbsz);
		if (ret) {
			ptio_dev_err(dev, "Prepare SCSI CDB failed\n");
			return ret;
		}
		break;
	case PTIO_CDB_ATA:
		if (!ptio_dev_is_ata(dev)) {
			ptio_dev_err(dev, "not an ATA device\n");
			return -EIO;
		}

		ret = ptio_ata_prepare_cdb(dev, cmd, cdb, cdbsz);
		if (ret) {
			ptio_dev_err(dev, "Prepare ATA CDB failed\n");
			return ret;
		}
		break;
	default:
		ptio_dev_err(dev, "Invalid CDB type\n");
		return -1;
	}

	if (ptio_verbose(dev)) {
		ptio_dev_info(dev, "Executing command, CDB %zu B, buffer %zu B:\n",
			      cmd->cdbsz, cmd->bufsz);
		ptio_print_cdb(cmd->cdb, cmd->cdbsz);
	}

	/* Setup SGIO header */
	cmd->io_hdr.interface_id = 'S';
	cmd->io_hdr.timeout = 30000;
	cmd->io_hdr.flags = 0x20; /* At head (at tail = 0x10)*/

	cmd->io_hdr.cmd_len = cmd->cdbsz;
	cmd->io_hdr.cmdp = cmd->cdb;

	cmd->io_hdr.dxferp = cmd->buf;
	cmd->io_hdr.dxfer_len = cmd->bufsz;
	cmd->io_hdr.dxfer_direction = sg_dxfer;

	cmd->io_hdr.mx_sb_len = PTIO_SENSE_MAX_LENGTH;
	cmd->io_hdr.sbp = cmd->sense_buf;

	/* Issue the command using SG_IO */
	ret = ioctl(dev->fd, SG_IO, &cmd->io_hdr);
	if (ret != 0) {
		ret = -errno;
		ptio_dev_err(dev, "SG_IO ioctl failed %d (%s)\n",
			     errno, strerror(errno));
		return ret;
	}

	ret = ptio_get_sense(dev, cmd);
	if (ret)
		return ret;

	if (cmd->io_hdr.resid) {
		ptio_dev_info(dev, "SCSI command residual: %u B\n",
			      cmd->io_hdr.resid);
		cmd->bufsz -= cmd->io_hdr.resid;
	}

	return 0;
}

/*
 * Test if a sysfs attribute file exists.
 */
static bool ptio_sysfs_exists(struct ptio_dev *dev, const char *format, ...)
{
	char path[PATH_MAX];
	struct stat st;
	va_list argp;
	int ret;

	va_start(argp, format);
	vsnprintf(path, sizeof(path) - 1, format, argp);
	va_end(argp);

	ret = stat(path, &st);
	if (ret == 0)
		return true;

	if (errno == ENOENT)
		return false;

	ptio_dev_err(dev, "stat %s failed %d (%s)\n",
		     path, errno, strerror(errno));

	return false;
}

/*
 * Get a sysfs attribute value.
 */
unsigned long ptio_sysfs_get_ulong_attr(struct ptio_dev *dev,
				       const char *format, ...)
{
	char path[PATH_MAX];
	unsigned long val = 0;
	va_list argp;
	FILE *f;

	va_start(argp, format);
	vsnprintf(path, sizeof(path) - 1, format, argp);
	va_end(argp);

	f = fopen(path, "r");
	if (f) {
		if (fscanf(f, "%lu", &val) != 1)
			val = 0;
		fclose(f);
	}

	return val;
}

/*
 * Set a sysfs attribute value.
 */
int ptio_sysfs_set_attr(struct ptio_dev *dev, const char *val,
		       const char *format, ...)
{
	char path[PATH_MAX];
	va_list argp;
	FILE *f;
	int ret = -1;

	va_start(argp, format);
	vsnprintf(path, sizeof(path) - 1, format, argp);
	va_end(argp);

	f = fopen(path, "w");
	if (f) {
		ret = fwrite(val, strlen(val), 1, f);
		if (ret <= 0)
			ret = -1;
		else
			ret = 0;
		fclose(f);
	}

	return ret;
}

/*
 * Allocate a command buffer.
 */
uint8_t *ptio_alloc_buf(size_t bufsz)
{
	void *buf;

	if (posix_memalign(&buf, sysconf(_SC_PAGESIZE), bufsz)) {
		fprintf(stderr, "Allocate %zu B buffer failed\n",
			bufsz);
		return NULL;
	}

	return buf;
}

uint8_t *ptio_read_buf(char *path, size_t *bufsz)
{
	struct stat st;
	uint8_t *buf;
	off_t sz = 0;
	ssize_t ret;
	int fd;

	if (stat(path, &st) < 0) {
		fprintf(stderr, "Get %s stat failed %d (%s)\n",
			path, errno, strerror(errno));
		return NULL;
	}

	buf = ptio_alloc_buf(st.st_size);
	if (!buf)
		return NULL;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Open %s failed %d (%s)\n",
			path, errno, strerror(errno));
		return NULL;
	}

	while (sz < st.st_size) {
		ret = read(fd, buf + sz, st.st_size - sz);
		if (ret <= 0) {
			fprintf(stderr, "Read %s failed %d (%s)\n",
				path, errno, strerror(errno));
			free(buf);
			buf = NULL;
			goto close;
		}
		sz += ret;
	}

	*bufsz = st.st_size;
close:
	close(fd);

	return buf;
}

int ptio_write_buf(char *path, uint8_t *buf, size_t bufsz)
{
	size_t sz = 0;
	ssize_t ret;
	int fd;

	fd = open(path, O_WRONLY | O_CREAT, 0644);
	if (fd < 0) {
		fprintf(stderr, "Open %s failed %d (%s)\n",
			path, errno, strerror(errno));
		return -1;
	}

	while (sz < bufsz) {
		ret = write(fd, buf + sz, bufsz - sz);
		if (ret <= 0) {
			fprintf(stderr, "Write %s failed %d (%s)\n",
				path, errno, strerror(errno));
			close(fd);
			return -1;
		}
		sz += ret;
	}

	close(fd);

	return 0;
}

/*
 * Open a device.
 */
int ptio_open_dev(struct ptio_dev *dev, enum ptio_dxfer dxfer)
{
	struct stat st;
	mode_t mode;

	/* Check that this is a block device */
	if (stat(dev->path, &st) < 0) {
		fprintf(stderr, "Get %s stat failed %d (%s)\n",
			dev->path, errno, strerror(errno));
		return -1;
	}

	if (!S_ISBLK(st.st_mode) && !S_ISCHR(st.st_mode)) {
		fprintf(stderr, "Invalid device file %s\n",
			dev->path);
		return -1;
	}

	/* Open device */
	switch (dxfer) {
	case PTIO_DXFER_TO_DEV:
		mode = O_RDWR;
		break;
	case PTIO_DXFER_NONE:
	case PTIO_DXFER_FROM_DEV:
		mode = O_RDONLY;
		break;
	default:
		fprintf(stderr, "Invalid dxfer type\n");
		return -1;
	}

	dev->fd = open(dev->path, mode | O_EXCL);
	if (dev->fd < 0) {
		fprintf(stderr, "Open %s failed %d (%s)\n",
			dev->path, errno, strerror(errno));
		return -1;
	}

	dev->name = basename(dev->path);

 	/* Check if this is an ATA device */
	if (ptio_sysfs_exists(dev, "/sys/block/%s/device/vpd_pg89", dev->name))
		dev->flags |= PTIO_ATA;

	return 0;
}

/*
 * Close an open device.
 */
void ptio_close_dev(struct ptio_dev *dev)
{
	if (dev->fd < 0)
		return;

	close(dev->fd);
	dev->fd = -1;
}

/*
 * Get a device information.
 */
int ptio_get_dev_information(struct ptio_dev *dev)
{
	int ret;

	ret = ptio_scsi_get_information(dev);
	if (ret)
		return ret;

	if (ptio_dev_is_ata(dev))
		return ptio_ata_get_information(dev);

	return 0;
}

/*
 * Revalidate a device: scsi device rescan does not trigger a revalidate in
 * libata. So for ATA devices managed with libata, always force a separate
 * ATA revalidate.
 */
int ptio_revalidate_dev(struct ptio_dev *dev)
{
	if (ptio_dev_is_ata(dev))
		return ptio_ata_revalidate(dev);

	return ptio_scsi_revalidate(dev);
}
