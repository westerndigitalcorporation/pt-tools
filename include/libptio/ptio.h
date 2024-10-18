// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SPDX-FileCopyrightText: 2024 Western Digital Corporation or its affiliates.
 *
 * Authors: Damien Le Moal (damien.lemoal@wdc.com)
 */
#ifndef LIBPTIO_H
#define LIBPTIO_H

#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <sys/types.h>
#include <scsi/sg.h>

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

/*
 * Device flags.
 */
#define PTIO_VERBOSE			(1 << 0)
#define PTIO_ATA			(1 << 1)

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

/*
 * Command flags.
 */

/* Force ATA PASSTHROUGH byte_block field to 0. */
#define PTIO_CMD_ATA_ZERO_BYTE_BLOCK	(1 << 0)
/* Force ATA PASSTHROUGH t_length to indicate number of LBAs */
#define PTIO_CMD_ATA_LBA_LEN		(1 << 1)

/*
 * Command descriptor.
 */
struct ptio_cmd {
	uint8_t			cdb[PTIO_CDB_MAX_SIZE];
	size_t			cdbsz;
	enum ptio_cdb_type	cdbtype;
	uint32_t		flags;

	uint8_t			*buf;
	size_t			bufsz;
	enum ptio_dxfer		dxfer;

	sg_io_hdr_t		io_hdr;

	uint8_t			sense_buf[PTIO_SENSE_MAX_LENGTH];
	uint8_t			sense_key;
	uint16_t		asc_ascq;
};

extern int ptio_open_dev(struct ptio_dev *dev, enum ptio_dxfer dxfer);
extern void ptio_close_dev(struct ptio_dev *dev);

extern int ptio_revalidate_dev(struct ptio_dev *dev);
extern int ptio_get_dev_information(struct ptio_dev *dev);
extern const char *ptio_ata_acs_ver(struct ptio_dev *dev);

extern int ptio_parse_cdb(char *cdb_str, uint8_t *cdb);

extern uint8_t *ptio_alloc_buf(size_t bufsz);
extern uint8_t *ptio_read_buf(char *path, size_t *bufsz);
extern int ptio_write_buf(char *path, uint8_t *buf, size_t bufsz);
extern void ptio_print_buf(uint8_t *buf, size_t bufsz);

extern int ptio_exec_cmd(struct ptio_dev *dev, struct ptio_cmd *cmd,
		uint8_t *cdb, size_t cdbsz, enum ptio_cdb_type cdb_type,
			 uint8_t *buf, size_t bufsz, enum ptio_dxfer dxfer,
			 uint32_t flags);

extern void ptio_print_sense(struct ptio_dev *dev,
			     uint8_t *sense, size_t sensesz);

static inline bool ptio_dev_is_ata(struct ptio_dev *dev)
{
	return dev->flags & PTIO_ATA;
}

/*
 * SCSI CDB and data access (big endian).
 */
static inline void ptio_set_be16(uint8_t *buf, uint16_t val)
{
	uint16_t *v = (uint16_t *)buf;

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	*v = __builtin_bswap16(val);
#else
	*v = val;
#endif
}

static inline uint16_t ptio_get_be16(uint8_t *buf)
{
	uint16_t val = *((uint16_t *)buf);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return __builtin_bswap16(val);
#else
	return val;
#endif
}

static inline void ptio_set_be32(uint8_t *buf, uint32_t val)
{
	uint32_t *v = (uint32_t *)buf;

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	*v = __builtin_bswap32(val);
#else
	*v = val;
#endif
}

static inline uint32_t ptio_get_be32(uint8_t *buf)
{
	uint32_t val = *((uint32_t *)buf);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return __builtin_bswap32(val);
#else
	return val;
#endif
}

static inline void ptio_set_be64(uint8_t *buf, uint64_t val)
{
	uint64_t *v = (uint64_t *)buf;

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	*v = __builtin_bswap64(val);
#else
	*v = val;
#endif
}

static inline uint64_t ptio_get_be64(uint8_t *buf)
{
	uint64_t val = *((uint64_t *)buf);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return __builtin_bswap64(val);
#else
	return val;
#endif
}

/*
 * ATA CDB and data access (little endian).
 */
static inline void ptio_set_le16(uint8_t *buf, uint16_t val)
{
	uint16_t *v = (uint16_t *)buf;

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	*v = val;
#else
	*v = __builtin_bswap16(val);
#endif
}

static inline uint16_t ptio_get_le16(uint8_t *buf)
{
	uint16_t val = *((uint16_t *)buf);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return val;
#else
	return __builtin_bswap16(val);
#endif
}

static inline void ptio_set_le32(uint8_t *buf, uint32_t val)
{
	uint32_t *v = (uint32_t *)buf;

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	*v = val;
#else
	*v = __builtin_bswap32(val);
#endif
}

static inline uint32_t ptio_get_le32(uint8_t *buf)
{
	uint32_t val = *((uint32_t *)buf);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return val;
#else
	return __builtin_bswap32(val);
#endif
}

static inline void ptio_set_le64(uint8_t *buf, uint64_t val)
{
	uint64_t *v = (uint64_t *)buf;

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	*v = val;
#else
	*v = __builtin_bswap64(val);
#endif
}

static inline uint64_t ptio_get_le64(uint8_t *buf)
{
	uint64_t val = *((uint64_t *)buf);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return val;
#else
	return __builtin_bswap64(val);
#endif
}

#endif /* LIBPTIO_H */
