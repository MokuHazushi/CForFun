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
 * Assumption :
 * Big endian representation
 *
 * The following macros correspond to a number of byte :
 * MBR_SIZE, 
 * NUMBER_PARTITION_RECORD, 
 * PARTITION_RECORD_SIZE,
 * SECTOR_SIZE
 *
 * REQUIRE ROOT ACCESS IF USING /dev/sda AS ARGUMENT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MBR_SIZE 		512
#define MBR_MAGIC_NUMBER_START	0x01fe
#define NUMBER_PARTITION_RECORD	4
#define PARTITION_RECORD_START 	0x01be
#define PARTITION_RECORD_SIZE	16
#define SECTOR_SIZE		512

/* ERROR CODES */
#define INPUT_ERROR 	0x0
#define IO_ERROR 	0x1
#define MEM_ERROR	0x2
#define FORMAT_ERROR	0x3

/* STRUCTS */
struct partition {
	char** os_indicator;
	double size; // in bytes
};

struct mbr {
	char devicename[128];
	struct partition partitions[NUMBER_PARTITION_RECORD];
};

/* VARIABLES */
char MAGIC_NUMBER[2] = {0x55, 0xaa};
unsigned int N_INDICATOR = 31;
char OS_INDICATORS[][128] =
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

/*
 * Convert a sequence of byte in its decimal representation
 * bin is the sequence of byte of size element
 *
 * Assumption : The binary sequence is not signed
 */
double binary_to_decimal(char *bin, unsigned int size)
{
	unsigned int i, j;
	double power;
	double result;

	result = 0;
	power = 0;
	for (i=0; i<size; i++) {
		for (j=0; j<8; j++) {
			result = result + ((bin[size-1-i] >> j) & 0x1) * pow(2, power);
			power += 1;
		}
	}
	return result;
}

/*
 * Parse the OS indicator code (incomplete list)
 * Return 'unknown' string if the corresponding code
 * is not found
 */
char* parse_os_indicator(char *osindicator)
{
	unsigned int i;
	unsigned int hexacode;
	char* error;

	for (i=0; i<N_INDICATOR; i++) {
		char* code = OS_INDICATORS[2*i];
		char* definition = OS_INDICATORS[(2*i)+1];

		if (strcmp(code, osindicator) == 0)
			return definition;
	}

	memcpy(&hexacode, osindicator, sizeof(char));
	printf("Warning: Unknown OS indicator '%02X'\n", hexacode);
	error = malloc(128*sizeof(char));
	strcat(error, "Unknown\0");
	return error;
}

/*
 * Parse one partition record starting at the partitionptr address
 *
 * Never fails, print errors in case of non parsable address
 * or unknown fields
 */
struct partition parse_partition(char *partitionptr)
{
	struct partition result;
	char *osindicator;
	char *partitionlgth;

	result.os_indicator = malloc(sizeof(char*));

	osindicator = malloc(sizeof(char));
	if (osindicator == NULL) {
		printf("Error: Could not allocate memory\n");
		exit(MEM_ERROR);
	}

	partitionlgth = malloc(SECTOR_SIZE * sizeof(char));
	if (partitionlgth == NULL) {
		printf("Error: Could not allocated memory\n");
		exit(MEM_ERROR);
	}

	memcpy(osindicator, partitionptr+0x4, sizeof(char));
	*result.os_indicator = parse_os_indicator(osindicator);

	memcpy(partitionlgth, partitionptr+0xc, SECTOR_SIZE*sizeof(char));
	result.size = binary_to_decimal(partitionlgth, SECTOR_SIZE);
	
	return result;
}

/*
 * Parse the MBR buffer starting at the mbrptr address
 *
 * Never fails, print errors in case of non parsable address
 * or unknown fields
 */
void parse_mbr(struct mbr *mbr, char *mbrptr)
{
	unsigned int i;
	
	mbrptr = mbrptr + PARTITION_RECORD_START;
	for (i=0; i<NUMBER_PARTITION_RECORD; i++)
		mbr->partitions[i] = parse_partition(mbrptr+i*PARTITION_RECORD_SIZE);
}

/*
 * Read the first 512 bytes of the given file.
 *
 * Exit in case of I/O error or the MBR could not be found
 */
char* read_mbr(char *fname)
{
	FILE *fptr;
	char *buffer;
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

	if (strcmp(MAGIC_NUMBER, buffer+MBR_MAGIC_NUMBER_START) != 0) {
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

void print_mbr(struct mbr *mbr)
{
	unsigned int i;

	printf("Device <%s> has master boot record (MBR) :\n", mbr->devicename);
	for (i=0; i<NUMBER_PARTITION_RECORD; i++) {
		struct partition partition = mbr->partitions[i];
		printf("\tPartition #%d:\n", i+1);
		printf("\t\tOperation system indicator: %s\n", *partition.os_indicator);
		printf("\t\tParition size (bytes): %f\n", partition.size);
	}
}

int main(int argc, char* argv[])
{
	struct mbr *mbr;
	char *mbrptr;

	if (argc != 2) {
		printf("Wrong number of arguments\n");
		print_usage();
		return INPUT_ERROR;
	}

	mbr = malloc(sizeof(struct mbr));
	if (mbr == NULL) {
		printf("Error: Could not allocate memory\n");
		exit(MEM_ERROR);
	}

	strcpy(mbr->devicename, argv[1]);
	mbrptr = read_mbr(argv[1]);
	parse_mbr(mbr, mbrptr);
	print_mbr(mbr);
}
