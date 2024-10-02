Copyright (C) 2024, Western Digital Corporation or its affiliates.

# <p align="center">Passthrough IO tools</p>

This project provides the *ptio* command line utility to execute SCSI or ATA
passthrough commands.

## License

The *pt-tools* project source code is distributed under the terms of the
GNU General Public License v2.0 or later
([GPL-v2](https://opensource.org/licenses/GPL-2.0)).
A copy of this license with *cdl-tools* copyright can be found in the files
[LICENSES/GPL-2.0-or-later.txt](LICENSES/GPL-2.0-or-later.txt) and
[COPYING.GPL](COPYING.GPL).

All source files in *pt-tools* contain the SPDX short identifier for the
GPL-2.0-or-later license in place of the full license text.

```
SPDX-License-Identifier: GPL-2.0-or-later
```

Some files such as the `Makefile.am` files and the `.gitignore` file are public
domain specified by the [CC0 1.0 Universal (CC0 1.0) Public Domain
Dedication](https://creativecommons.org/publicdomain/zero/1.0/).
These files are identified with the following SPDX short identifier header.

```
SPDX-License-Identifier: CC0-1.0
```

See [LICENSES/CC0-1.0.txt](LICENSES/CC0-1.0.txt) for the full text of this
license.

## Requirements

The following packages must be installed prior to compiling *pt-tools*.

* autoconf
* autoconf-archive
* automake
* libtool

## Compilation and Installation

The following commands will compile the *ptio* utility.

```
$ sh ./autogen.sh
$ ./configure
$ make
```

To install the compiled executable file and the man page for the *ptio*
utility, the following command can be used.

```
$ sudo make install
```

The default installation directory is /usr/bin. This default location can be
changed using the configure script. Executing the following command displays
the options used to control the installation path.

```
$ ./configure --help
```

## Building RPM Packages

The *rpm* and *rpmbuild* utilities are necessary to build *pt-tools* RPM
packages. Once these utilities are installed, the RPM packages can be built
using the following command.

```
$ sh ./autogen.sh
$ ./configure
$ make rpm
```

Five RPM packages are built: a binary package providing *ptio* executable
and its documentation, a source RPM package, a *debuginfo*
RPM package and a *debugsource* RPM package, and an RPM package containing the
test suite.

The source RPM package can be used to build the binary and debug RPM packages
outside of *pt-tools* source tree using the following command.

```
$ rpmbuild --rebuild pt-tools-<version>.src.rpm
```

## Contributing

Read the [CONTRIBUTING](CONTRIBUTING) file and send patches to:

	Damien Le Moal <damien.lemoal@wdc.com>
	Niklas Cassel <niklas.cassel@wdc.com>

# *ptio* Command Line Tool

The *ptio* command line tool allows executing SCSI or ATA passthrough commands.

## Usage

*ptio* provides many functions. The usage is as follows:

```
$ ptio --help
Usage:
  ptio --help | -h
  ptio --version
  ptio [options] <device>
Options:
  --verbose | -v   : Verbose output.
  --info           : Display device information and return.
  --revalidate     : Revalidate the device and return.
  --scsi-cdb <str> : Space separated hexadecimal string
                     defining a SCSI cdb.
  --ata-cdb <str>  : Space separated hexadecimal string
                     defining a 28-bits 0r 48-bits ATA cdb.
  --in-buf <path>  : Use the file <path> as the command input
                     buffer. The file size will be used as the
                     buffer size.
  --out-buf <path> : Save the command output buffer to the file
                     specified by <path>
  --bufsz <sz>     : Specify the size of the command buffer
                     (default: 0). This option is ignored if
                     --in-buf is used.
  --to-dev         : Specify that the command transfers data
                     from the host to the device.
  --from-dev       : Data transfer from device to host.
See "man ptio" for more information.
```

## SCSI Command Descriptor Block

*ptio* can be used to execute a SCSI command by specifying the command CDB as a
string of hexadecimals, with each hexadecimal value in the string representing
one byte of the CDB. E.g For an INQUIRY command with a 6 B CDB, the sting has 6
hexadecimal values: "12 01 00 02 00 00". The first value (byte 0 of the CDB),
specifies the INQUIRY command operation code (12h).

6-bytes, 10-bytes, 16-bytes and 32-bytes command descriptors are supported.

## ATA Command Descriptor Block

For ATA commands, the hexadecimal value string specifies the field, count, lba,
device and command fields of the ATA command. The ICC and AUXILIARY fields are
not supported.

The length of the CDB fields changes depending on the command type.

For 28-bits ATA commands, the field, count, device and command fields are all
one byte (single HEX value). The LBA field uses 4 bytes (4 HEX values). That is,
for a 28-bits ATA command, the CDB string always has 8 values for the format:

"feat cnt lba[27:24] lba[23:16] lba[15:8] lba[0:7] dev cmd"

For a 48-bits ATA command, the features and count fields become 16-bits (2
bytes) and the lba field has 48 bits (6 bytes) with the format:

"feat[15:8] feat[7:0] cnt[15:8] cnt[7:0] lba[47:40] lba[39:32] lba[31:24] lba[23:16] lba[15:8] lba[0:7] dev cmd"

## Examples

Execute the INQUIRY command for page 0h:

```
$ sudo ptio --bufsz 512 --from-dev --scsi-cdb "12 01 00 02 00 00" /dev/sda
Executing command, CDB 6 B, buffer 512 B:
  +----------+-------------------------------------------------+
  |  OFFSET  | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |
  +----------+-------------------------------------------------+
  | 00000000 | 12 01 00 02 00 00                               |
  +----------+-------------------------------------------------+
sda: SCSI command residual: 499 B
Command result 13 Bytes:
  +----------+-------------------------------------------------+
  |  OFFSET  | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |
  +----------+-------------------------------------------------+
  | 00000000 | 00 00 00 09 00 80 83 87 89 8a b0 b1 b2          |
  +----------+-------------------------------------------------+
```

Execute the IDENTIFY ATA command:

```
$ sudo ptio --bufsz 512 --from-dev --ata-cdb "00 00 00 00 00 00 00 EC" /dev/sdc
Command result 512 Bytes:
  +----------+-------------------------------------------------+
  |  OFFSET  | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |
  +----------+-------------------------------------------------+
  | 00000000 | 5a 04 ff 3f 37 c8 10 00 00 00 00 00 3f 00 00 00 |
  | 00000010 | 00 00 00 00 35 36 4d 47 35 4e 4e 47 20 20 20 20 |
  | 00000020 | 20 20 20 20 20 20 20 20 03 00 00 00 38 00 56 4c |
  | 00000030 | 4d 47 46 57 30 31 44 57 20 43 57 20 48 53 32 37 |
  | 00000040 | 38 32 30 38 4c 41 36 4e 34 30 20 20 20 20 20 20 |
  | 00000050 | 20 20 20 20 20 20 20 20 20 20 20 20 20 20 02 80 |
  | 00000060 | 00 40 00 2f 00 40 00 02 00 02 07 00 ff 3f 10 00 |
  | 00000070 | 3f 00 10 fc fb 00 00 59 ff ff ff 0f 00 00 07 00 |
  | 00000080 | 03 00 78 00 78 00 78 00 78 00 08 0d 00 00 00 00 |
  | 00000090 | 00 00 00 00 00 00 1f 00 0e 97 76 01 de 1c 44 04 |
  | 000000a0 | fc 1f 00 00 6b 74 69 7d 63 41 69 74 41 bc 63 41 |
  | 000000b0 | 7f 40 4e 85 00 00 fe 00 fe ff 00 00 00 00 00 00 |
  | 000000c0 | 00 00 00 00 00 00 00 00 00 00 78 97 01 00 00 00 |
  | 000000d0 | 00 00 00 00 00 50 87 5a 00 50 a2 cc c8 f7 ca ee |
  | 000000e0 | 00 00 00 00 00 00 00 00 00 00 00 08 00 00 dc 40 |
  | 000000f0 | dc 40 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000100 | 01 00 0b 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000110 | 00 c0 00 80 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000120 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 09 00 |
  | 00000130 | 4e 47 00 00 00 00 00 00 00 00 57 53 33 4e 30 36 |
  | 00000140 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000150 | 02 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000160 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000170 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000180 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000190 | 00 00 00 00 00 00 00 00 00 00 00 00 3d 00 00 00 |
  | 000001a0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000001b0 | 00 00 20 1c 00 00 00 00 00 00 00 00 ff 17 00 00 |
  | 000001c0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 78 97 |
  | 000001d0 | 01 00 00 00 08 00 00 00 00 00 00 00 00 00 00 00 |
  | 000001e0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000001f0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 a5 11 |
  +----------+-------------------------------------------------+
```

Adding the *--verbose* option to the same command allows seeing the ATA 16
SCSI command used for execution:

```
sudo ptio --verbose --bufsz 512 --from-dev --ata-cdb "00 00 00 00 00 00 00 EC" /dev/sdc
ATA Command: IDENTIFY_DEVICE
sdc: Executing command, CDB 16 B, buffer 512 B:
  +----------+-------------------------------------------------+
  |  OFFSET  | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |
  +----------+-------------------------------------------------+
  | 00000000 | 85 08 0e 00 00 00 00 00 00 00 00 00 00 00 ec 00 |
  +----------+-------------------------------------------------+
Command result 512 Bytes:
  +----------+-------------------------------------------------+
  |  OFFSET  | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |
  +----------+-------------------------------------------------+
  | 00000000 | 5a 04 ff 3f 37 c8 10 00 00 00 00 00 3f 00 00 00 |
  | 00000010 | 00 00 00 00 35 36 4d 47 35 4e 4e 47 20 20 20 20 |
  | 00000020 | 20 20 20 20 20 20 20 20 03 00 00 00 38 00 56 4c |
  | 00000030 | 4d 47 46 57 30 31 44 57 20 43 57 20 48 53 32 37 |
  | 00000040 | 38 32 30 38 4c 41 36 4e 34 30 20 20 20 20 20 20 |
  | 00000050 | 20 20 20 20 20 20 20 20 20 20 20 20 20 20 02 80 |
  | 00000060 | 00 40 00 2f 00 40 00 02 00 02 07 00 ff 3f 10 00 |
  | 00000070 | 3f 00 10 fc fb 00 00 59 ff ff ff 0f 00 00 07 00 |
  | 00000080 | 03 00 78 00 78 00 78 00 78 00 08 0d 00 00 00 00 |
  | 00000090 | 00 00 00 00 00 00 1f 00 0e 97 76 01 de 1c 44 04 |
  | 000000a0 | fc 1f 00 00 6b 74 69 7d 63 41 69 74 41 bc 63 41 |
  | 000000b0 | 7f 40 4e 85 00 00 fe 00 fe ff 00 00 00 00 00 00 |
  | 000000c0 | 00 00 00 00 00 00 00 00 00 00 78 97 01 00 00 00 |
  | 000000d0 | 00 00 00 00 00 50 87 5a 00 50 a2 cc c8 f7 ca ee |
  | 000000e0 | 00 00 00 00 00 00 00 00 00 00 00 08 00 00 dc 40 |
  | 000000f0 | dc 40 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000100 | 01 00 0b 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000110 | 00 c0 00 80 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000120 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 09 00 |
  | 00000130 | 4e 47 00 00 00 00 00 00 00 00 57 53 33 4e 30 36 |
  | 00000140 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000150 | 02 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000160 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000170 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000180 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000190 | 00 00 00 00 00 00 00 00 00 00 00 00 3d 00 00 00 |
  | 000001a0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000001b0 | 00 00 20 1c 00 00 00 00 00 00 00 00 ff 17 00 00 |
  | 000001c0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 78 97 |
  | 000001d0 | 01 00 00 00 08 00 00 00 00 00 00 00 00 00 00 00 |
  | 000001e0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000001f0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 a5 11 |
  +----------+-------------------------------------------------+
```

And the same IDENTIFY command can be issued as an ATA 16 passthrough SCSI
command:

```
$ sudo ptio --bufsz 512 --from-dev --scsi-cdb ""85 08 0e 00 00 00 00 00 00 00 00 00 00 00 EC 00 /dev/sdc
Command result 512 Bytes:
  +----------+-------------------------------------------------+
  |  OFFSET  | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |
  +----------+-------------------------------------------------+
  | 00000000 | 5a 04 ff 3f 37 c8 10 00 00 00 00 00 3f 00 00 00 |
  | 00000010 | 00 00 00 00 35 36 4d 47 35 4e 4e 47 20 20 20 20 |
  | 00000020 | 20 20 20 20 20 20 20 20 03 00 00 00 38 00 56 4c |
  | 00000030 | 4d 47 46 57 30 31 44 57 20 43 57 20 48 53 32 37 |
  | 00000040 | 38 32 30 38 4c 41 36 4e 34 30 20 20 20 20 20 20 |
  | 00000050 | 20 20 20 20 20 20 20 20 20 20 20 20 20 20 02 80 |
  | 00000060 | 00 40 00 2f 00 40 00 02 00 02 07 00 ff 3f 10 00 |
  | 00000070 | 3f 00 10 fc fb 00 00 59 ff ff ff 0f 00 00 07 00 |
  | 00000080 | 03 00 78 00 78 00 78 00 78 00 08 0d 00 00 00 00 |
  | 00000090 | 00 00 00 00 00 00 1f 00 0e 97 76 01 de 1c 44 04 |
  | 000000a0 | fc 1f 00 00 6b 74 69 7d 63 41 69 74 41 bc 63 41 |
  | 000000b0 | 7f 40 4e 85 00 00 fe 00 fe ff 00 00 00 00 00 00 |
  | 000000c0 | 00 00 00 00 00 00 00 00 00 00 78 97 01 00 00 00 |
  | 000000d0 | 00 00 00 00 00 50 87 5a 00 50 a2 cc c8 f7 ca ee |
  | 000000e0 | 00 00 00 00 00 00 00 00 00 00 00 08 00 00 dc 40 |
  | 000000f0 | dc 40 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000100 | 01 00 0b 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000110 | 00 c0 00 80 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000120 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 09 00 |
  | 00000130 | 4e 47 00 00 00 00 00 00 00 00 57 53 33 4e 30 36 |
  | 00000140 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000150 | 02 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000160 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000170 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000180 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000190 | 00 00 00 00 00 00 00 00 00 00 00 00 3d 00 00 00 |
  | 000001a0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000001b0 | 00 00 20 1c 00 00 00 00 00 00 00 00 ff 17 00 00 |
  | 000001c0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 78 97 |
  | 000001d0 | 01 00 00 00 08 00 00 00 00 00 00 00 00 00 00 00 |
  | 000001e0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000001f0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 a5 11 |
  +----------+-------------------------------------------------+
```

NCQ commands can also be executed. For instance, the following command reads
an ATA device statistics log page using an NCQ read log command:

```
sudo ptio --bufsz 512 --from-dev --ata-cdb "00 01 01 00 00 00 00 00 05 04 40 65" /dev/sdc
Command result 512 Bytes:
  +----------+-------------------------------------------------+
  |  OFFSET  | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |
  +----------+-------------------------------------------------+
  | 00000000 | 01 00 05 00 00 00 00 00 1f 00 00 00 00 00 00 c0 |
  | 00000010 | 1d 00 00 00 00 00 00 e0 1c 00 00 00 00 00 00 e0 |
  | 00000020 | 20 00 00 00 00 00 00 c0 18 00 00 00 00 00 00 c0 |
  | 00000030 | 1e 00 00 00 00 00 00 e0 19 00 00 00 00 00 00 e0 |
  | 00000040 | 1d 00 00 00 00 00 00 e0 19 00 00 00 00 00 00 e0 |
  | 00000050 | 00 00 00 00 00 00 00 c0 3c 00 00 00 00 00 00 c0 |
  | 00000060 | 00 00 00 00 00 00 00 c0 00 00 00 00 00 00 00 c0 |
  | 00000070 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000080 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000090 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000000a0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000000b0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000000c0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000000d0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000000e0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000000f0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000100 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000110 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000120 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000130 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000140 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000150 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000160 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000170 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000180 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000190 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000001a0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000001b0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000001c0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000001d0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000001e0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000001f0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  +----------+-------------------------------------------------+
```

And the same action can also be executed using a SCSI ATA 16 command:

```
sudo ptio --bufsz 512 --from-dev --scsi-cdb "85 19 0D 00 01 01 00 00 04 00 05 00 00 40 65 00" /dev/sdc
Command result 512 Bytes:
  +----------+-------------------------------------------------+
  |  OFFSET  | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |
  +----------+-------------------------------------------------+
  | 00000000 | 01 00 05 00 00 00 00 00 1e 00 00 00 00 00 00 c0 |
  | 00000010 | 1c 00 00 00 00 00 00 e0 1c 00 00 00 00 00 00 e0 |
  | 00000020 | 20 00 00 00 00 00 00 c0 18 00 00 00 00 00 00 c0 |
  | 00000030 | 1e 00 00 00 00 00 00 e0 19 00 00 00 00 00 00 e0 |
  | 00000040 | 1d 00 00 00 00 00 00 e0 19 00 00 00 00 00 00 e0 |
  | 00000050 | 00 00 00 00 00 00 00 c0 3c 00 00 00 00 00 00 c0 |
  | 00000060 | 00 00 00 00 00 00 00 c0 00 00 00 00 00 00 00 c0 |
  | 00000070 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000080 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000090 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000000a0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000000b0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000000c0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000000d0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000000e0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000000f0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000100 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000110 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000120 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000130 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000140 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000150 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000160 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000170 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000180 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 00000190 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000001a0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000001b0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000001c0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000001d0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000001e0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  | 000001f0 | 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 |
  +----------+-------------------------------------------------+
```
