/*
 * This program reads the Master Boot Record and analyse the partition entries
 *
 * Usage :
 * sudo ./partition_record_reader <device>
 *
 * Where <device> is the OS hard-disk mounting point
 *
 * Example :
 * sudo ./partition_record_reader /dev/sda
 *
 * REQUIRE ROOT ACCESS
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MBR_SIZE 		512
#define MBR_MAGIC_NUMBER_START	0x01fe
#define NUMBER_PARTITION_RECORD	4
#define PARTITION_RECORD_START 	0x01be
#define PARTITION_RECORD_SIZE	16

/* ERROR CODES */
#define INPUT_ERROR 	0x0
#define IO_ERROR 	0x1
#define MEM_ERROR	0x2
#define FORMAT_ERROR	0x3

/* STRUCTS */
struct partition {
	char* os_indicator;
	unsigned long size; // in bytes
};

struct mbr {
	char* devicename;
	struct partition partitions[NUMBER_PARTITION_RECORD];
};

/* VARIABLES */
char* mbrptr;
struct mbr* mbr;
char magicnumber[2] = {0x55, 0xaa};

/*
 * Parse the OS indicator code (incomplete list)
 * Return 'unknown' string if the corresponding code
 * is not found
 */
char* parse_os_indicator(char* osindicator)
{
	unsigned int nindicator = 31;
	unsigned int i;
	char* result;
	unsigned int hexacode;
	char indicators[][128] =
	{
	       	{0x00}, {"Empty partition-table entry"},
		{0x01}, {"DOS FAT12"},
		{0x04}, {"DOS FAT16 (up to 32MB)"},
		{0x05}, {"DOS 3.3+ extended partition"},
		{0x06}, {"DOS3.31+ FAT16 (over 32MB)"},

		{0x07}, {"OS/2 HPFS, Windows NT NTFS, Advanced Unix"},
		{0x08}, {"OS/2 v1.0-1.3, AIX bootable partition, SplitDrive"},
		{0x09}, {"AIX data partition"},
		{0x0a}, {"OS/2 Boot Manager"},
		{0x0b}, {"Windows 95+ FAT32"},

		{0x0c}, {"Windows 95+ FAT32 (using LBA-mode INT 13 extensions)"},
		{0x0e}, {"DOS FAT16 (over 32MB, using INT 13 extensions)"},
		{0x0f}, {"Extended partition (using INT 13 extensions)"},
		{0x17}, {"Hidden NTFS partition"},
		{0x1b}, {"Hidden Windows 95 FAT32 partition"},

		{0x1c}, {"Hidden Windows 95 FAT32 partition (using LBA-mode INT 13 extensions)"},
		{0x1e}, {"Hidden LBA VFAT partition"},
		{0x42}, {"Dynamic disk volume"},
		{0x50}, {"OnTrack Disk Manager, read-only partition"},
		{0x51}, {"OnTrack Disk Manager, read/write partition"},

		{0x81}, {"Linux"},
		{0x82}, {"Linux Swap partition, Solaris (Unix)"},
		{0x83}, {"Linux native file system (ext2fs/xiafs)"},
		{0x85}, {"Linux EXT"},
		{0x86}, {"FAT16 volume/stripe set (Windows NT)"},

		{0x87}, {"HPFS fault-tolerant mirrored partition, NTFS volume/stripe set"},
		{0xbe}, {"Solaris boot partition"},
		{0xc0}, {"DR-DOS/Novell DOS secured partition"},
		{0xc6}, {"Corrupted FAT16 volume/stripe set (Windows NT)"},
		{0xc7}, {"Corrupted NTFS volume/stripe set"},

		{0xf2}, {"DOS 3.3+ secondary partition"}
	};
	
	result = malloc(128*sizeof(char));

	for (i=0; i<nindicator; i++) {
		char* code = indicators[2*i];
		char* definition = indicators[(2*i)+1];

		if (strcmp(code, osindicator) == 0) {
			memcpy(result, definition, 128*sizeof(char));
			return result;
		}
	}

	memcpy(&hexacode, osindicator, sizeof(char));
	printf("Warning: Unknown OS indicator '%02X'\n", hexacode);
	strcat(result, "Unknown");
	return result;
}

/*
 * Parse one partition record starting at the partitionptr address
 *
 * Never fails, print errors in case of non parsable address
 * or unknown fields
 */
struct partition parse_partition(char* partitionptr)
{
	struct partition result;
	char* starthead, *endhead;
	char* osindicator;

	osindicator = malloc(sizeof(char));
	if (osindicator == NULL) {
		printf("Error: Could not allocate memory\n");
		exit(MEM_ERROR);
	}
	memcpy(osindicator, partitionptr+0x4, sizeof(char));
	result.os_indicator = parse_os_indicator(osindicator);
}

/*
 * Parse the MBR buffer starting at the mbrptr address
 *
 * Never fails, print errors in case of non parsable address
 * or unknown fields
 */
void parse_mbr(struct mbr* mbr, char* mbrptr)
{
	unsigned i;
	
	mbrptr = mbrptr + PARTITION_RECORD_START;
	for (int i=0; i<NUMBER_PARTITION_RECORD; i++) {
		mbr->partitions[i] = parse_partition(mbrptr+i*PARTITION_RECORD_SIZE);
	}
}

/*
 * Read the first 512 bytes of the given file.
 *
 * Exit in case of I/O error or the MBR could not be found
 */
char* read_mbr(char* fname)
{
	FILE* fptr;
	char* buffer;
	size_t result;

	buffer = malloc(MBR_SIZE * sizeof(char));
	if (buffer == NULL) {
		printf("Error: Could not allocate memory\n");
		exit(MEM_ERROR);
	}

	fptr = fopen(fname, "rb");
	if (fptr == NULL) {
		printf("Error: Could not open file '%s'\n", fname);
		exit(IO_ERROR);
	}

	result = fread(buffer, sizeof(char), MBR_SIZE, fptr);
	if (result != MBR_SIZE * sizeof(char)) {
		printf("Error: Error while reading the file '%s'\n", fname);
		exit(IO_ERROR);
	}

	if (strcmp(magicnumber, buffer+MBR_MAGIC_NUMBER_START) != 0) {
		printf("Error: the found buffer is not a MBR\n");
		exit(FORMAT_ERROR);
	}

	return buffer;
}

void print_usage()
{
	printf("Usage :\nsudo ./partition_record_reader <device>\n");
	printf("(Usually <device> is /dev/sda)\n");
}

void print_mbr(struct mbr* mbr)
{
	unsigned int i;

	printf("Device <%s> has master boot record (MBR) :\n", mbr->devicename);
	for (i=0; i<NUMBER_PARTITION_RECORD; i++) {
		struct partition partition = mbr->partitions[i];
		printf("\tPartition #%d:\n", i+1);
		printf("\t\tOperation system indicator: %s\n", partition.os_indicator);
	}
}

int main(int argc, char* argv[])
{
	mbr = malloc(sizeof(struct mbr));
	if (mbr == NULL) {
		printf("Error: Could not allocate memory\n");
		exit(MEM_ERROR);
	}

	if (argc != 2) {
		printf("Wrong number of arguments\n");
		print_usage();
		return INPUT_ERROR;
	}

	mbrptr = read_mbr(argv[1]);
	mbr->devicename = argv[1];
	parse_mbr(mbr, mbrptr);
	print_mbr(mbr);
}
