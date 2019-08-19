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

#define INPUT_ERROR 0x1

/*
 * Read the first 512 bytes of the given file.
 *
 * Return null in case of I/O error or the MBR could not be found
 */
char* read_mbr(FILE* filept)
{
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
}
