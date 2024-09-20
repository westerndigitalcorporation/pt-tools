// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024 Western Digital Corporation or its affiliates.
 *
 * Authors: Damien Le Moal (damien.lemoal@wdc.com)
 */

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/utsname.h>

#include "ptio.h"

static int ptio_information(struct ptio_dev *dev)
{
	int ret;

	ret = ptio_get_dev_information(dev);
	if (ret) {
		ptio_dev_err(dev, "Get device information failed\n");
		return ret;
	}

	printf("Device: /dev/%s\n", dev->name);
	printf("    Vendor: %s\n", dev->vendor);
	printf("    Product: %s\n", dev->id);
	printf("    Revision: %s\n", dev->rev);
	printf("    %llu 512-byte sectors (%llu.%03llu TB)\n",
	       dev->capacity,
	       (dev->capacity << 9) / 1000000000000,
	       ((dev->capacity << 9) % 1000000000000) / 1000000000);
	printf("    Device interface: %s\n",
	       ptio_dev_is_ata(dev) ? "ATA" : "SAS");
	if (ptio_dev_is_ata(dev)) {
		printf("      ACS version: %s\n", ptio_ata_acs_ver(dev));
		printf("      SAT Vendor: %s\n", dev->sat_vendor);
		printf("      SAT Product: %s\n", dev->sat_product);
		printf("      SAT revision: %s\n", dev->sat_rev);
	}

	return 0;
}


static int ptio_revalidate(struct ptio_dev *dev)
{
	int ret;

	ret = ptio_revalidate_dev(dev);
	if (ret) {
		ptio_dev_err(dev, "Revalidate failed\n");
		return ret;
	}

	return 0;
}

static int ptio_exec(struct ptio_dev *dev, char *cdb_str,
		     enum ptio_cdb_type cdb_type, int dxfer,
		     char *buf_path, size_t bufsz)
{
	struct ptio_cmd cmd;
	uint8_t cdb[PTIO_CDB_MAX_SIZE];
	uint8_t *buf = NULL;
	int ret, cdbsz;

	/* Parse the command cdb */
	if (!cdb_str) {
		fprintf(stderr, "No CDB specified\n");
		return -1;
	}

	cdbsz = ptio_parse_cdb(cdb_str, cdb);
	if (cdbsz <= 0) {
		fprintf(stderr, "Invalid CDB\n");
		return -1;
	}

	/* Get a buffer if needed */
	if (buf_path && dxfer == PTIO_DXFER_TO_DEV)
		buf = ptio_read_buf(buf_path, &bufsz);
	else if (dxfer != PTIO_DXFER_NONE)
		buf = ptio_alloc_buf(bufsz);
	if (!buf)
		return -1;

	/* Execute the command */
	ret = ptio_exec_cmd(dev, &cmd, cdb, cdbsz, cdb_type, buf, bufsz, dxfer);
	if (ret) {
		ptio_dev_err(dev, "Command failed\n");
		return ret;
	}

	if (dxfer == PTIO_DXFER_FROM_DEV) {
		if (buf_path) {
			ret = ptio_write_buf(buf_path, buf, cmd.bufsz);
			if (ret)
				return ret;
			printf("Command result %zu Bytes written to %s\n",
			       cmd.bufsz, buf_path);
		} else {
			printf("Command result %zu Bytes:\n", cmd.bufsz);
			ptio_print_buf(buf, cmd.bufsz);
		}
	}

	return 0;
}

/*
 * Print usage.
 */
static void ptio_usage(void)
{
	printf("Usage:\n"
	       "  ptio --help | -h\n"
	       "  ptio --version\n"
	       "  ptio [options] <device>\n");
	printf("Options:\n"
	       "  --verbose | -v   : Verbose output.\n"
	       "  --info           : Display device information and return.\n"
	       "  --revalidate     : Revalidate the device and return.\n"
	       "  --scsi-cdb <str> : Space separated hexadecimal string\n"
	       "                     defining a SCSI cdb.\n"
	       "  --ata-cdb <str>  : Space separated hexadecimal string\n"
	       "                     defining a 28-bits 0r 48-bits ATA cdb.\n"
	       "  --in-buf <path>  : Use the file <path> as the command input\n"
	       "                     buffer. The file size will be used as the\n"
	       "                     buffer size.\n"
	       "  --out-buf <path> : Save the command output buffer to the file\n"
	       "                     specified by <path>\n"
	       "  --bufsz <sz>     : Specify the size of the command buffer\n"
	       "                     (default: 0). This option is ignored if\n"
	       "                     --in-buf is used.\n"
	       "  --to-dev         : Specify that the command transfers data\n"
	       "                     from the host to the device.\n"
	       "  --from-dev       : Data transfer from device to host.\n");
	printf("See \"man ptio\" for more information.\n");
}

/*
 * Main function.
 */
int main(int argc, char **argv)
{
	struct ptio_dev dev;
	char *cdb_str = NULL;
	enum ptio_cdb_type cdb_type = PTIO_CDB_NONE;
	int dxfer = PTIO_DXFER_NONE;
	char *buf_path = NULL;
	int bufsz = 0;
	int i, ret;

	/* Initialize */
	memset(&dev, 0, sizeof(dev));
	dev.fd = -1;

	if (argc == 1) {
		ptio_usage();
		return 0;
	}

	/* Generic options */
	if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
		ptio_usage();
		return 0;
	}

	if (strcmp(argv[1], "--version") == 0) {
		printf("ptio, version %s\n", PACKAGE_VERSION);
		printf("Copyright (C) 2024, Western Digital Corporation"
		       " or its affiliates.\n");
		return 0;
	}

	/* Parse options */
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--verbose") == 0 ||
		    strcmp(argv[i], "-v") == 0) {
			dev.flags |= PTIO_VERBOSE;
			continue;
		}

		if (strcmp(argv[i], "--info") == 0) {
			dev.flags |= PTIO_INFO;
			continue;
		}

		if (strcmp(argv[i], "--revalidate") == 0) {
			dev.flags |= PTIO_REVALIDATE;
			continue;
		}

		if (strcmp(argv[i], "--scsi-cdb") == 0) {
			if (cdb_str) {
				fprintf(stderr, "CDB specified multiple times\n");
				return -1;
			}
			i++;
			if (i >= argc)
				goto invalid_cmdline;
			cdb_str = argv[i];
			cdb_type = PTIO_CDB_SCSI;
			continue;
		}

		if (strcmp(argv[i], "--ata-cdb") == 0) {
			if (cdb_str) {
				fprintf(stderr, "CDB specified multiple times\n");
				return -1;
			}
			i++;
			if (i >= argc)
				goto invalid_cmdline;
			cdb_str = argv[i];
			cdb_type = PTIO_CDB_ATA;
			continue;
		}

		if (strcmp(argv[i], "--in-buf") == 0) {
			i++;
			if (i >= argc)
				goto invalid_cmdline;
			buf_path = argv[i];
			continue;
		}

		if (strcmp(argv[i], "--out-buf") == 0) {
			i++;
			if (i >= argc)
				goto invalid_cmdline;
			buf_path = argv[i];
			continue;
		}

		if (strcmp(argv[i], "--bufsz") == 0) {
			i++;
			if (i >= argc)
				goto invalid_cmdline;
			bufsz = atoi(argv[i]);
			if (bufsz <= 0) {
				fprintf(stderr, "Invalid buffer size\n");
				return -1;
			}
			continue;
		}

		if (strcmp(argv[i], "--to-dev") == 0) {
			dxfer = PTIO_DXFER_TO_DEV;
			continue;
		}

		if (strcmp(argv[i], "--from-dev") == 0) {
			dxfer = PTIO_DXFER_FROM_DEV;
			continue;
		}

		if (argv[i][0] != '-')
			break;

		fprintf(stderr, "Invalid option '%s'\n", argv[i]);
		return 1;
	}

	if (i != argc - 1) {
invalid_cmdline:
		fprintf(stderr, "Invalid command line\n");
		return 1;
	}

	/* Get device path */
	dev.path = realpath(argv[i], NULL);
	if (!dev.path) {
		fprintf(stderr, "Failed to get device real path\n");
		return 1;
	}

	/* Open the device and printf some information about it */
	ret = ptio_open_dev(&dev, O_RDWR);
	if (ret)
		return 1;

	if (dev.flags & PTIO_INFO)
		ret = ptio_information(&dev);
	else if (dev.flags & PTIO_REVALIDATE)
		ret = ptio_revalidate(&dev);
	else
		ret = ptio_exec(&dev, cdb_str, cdb_type, dxfer, buf_path, bufsz);

	ptio_close_dev(&dev);

	if (ret)
		return 1;

	return 0;
}

