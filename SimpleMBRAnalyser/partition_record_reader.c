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
#define PARTITION_RECORD_START 	0x01be
#define PARTITION_RECORD_SIZE	16

/* ERROR CODES */
#define INPUT_ERROR 	0x0
#define IO_ERROR 	0x1
#define MEM_ERROR	0x2
#define FORMAT_ERROR	0x3

/* STRUCTS */
struct partition {
	char os_indicator[128];
	unsigned long size; // in bytes
};

struct mbr {
	char* devicename;
	struct partition partitions[4];
};

/* VARIABLES */
char* mbrptr;
struct mbr mbr;
char magicnumber[2] = {0x55, 0xaa};

/*
 * Parse one partition record starting at the mbrptr address
 *
 * Never fails, print errors in case of non parsable address
 * or unknown fields
 */
struct partition parse_partition(char* mbrptr)
{
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
}

void print_usage()
{
	printf("Usage :\nsudo ./partition_record_reader <device>\n");
	printf("(Usually <device> is /dev/sda)\n");
}

int main(int argc, char* argv[])
{
	if (argc != 2) {
		printf("Wrong number of arguments\n");
		print_usage();
		return INPUT_ERROR;
	}

	mbrptr = read_mbr(argv[1]);
}
