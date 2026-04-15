#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct zip_header
{
	uint16_t version_needed_to_extract;
	uint16_t general_purpose_bit_flag;
	uint16_t compression_method;
	uint16_t last_mod_file_time;
	uint16_t last_mod_file_date;
	uint32_t crc_32;
	uint32_t compressed_size;
	uint32_t uncompressed_size;
	uint16_t file_name_length;
	uint16_t extra_field_length;
	size_t file_name;
	size_t extra_field;
};

struct central_directory_structure
{
	uint16_t version_made_by;
	uint16_t version_needed_to_extract;
	uint16_t general_purpose_bit_flag;
	uint16_t compression_method;
	uint16_t last_mod_file_time;
	uint16_t last_mod_file_date;
	uint32_t crc_32;
	uint32_t compressed_size;
	uint32_t uncompressed_size;
	uint16_t file_name_length;
	uint16_t extra_field_length;
	uint16_t file_comment_length;
	uint16_t disk_number_start;
	uint16_t internal_file_attributes;
	uint32_t external_file_attributes;
	uint32_t relative_offset_of_local_header;
	size_t file_name;
	size_t extra_field;
	size_t file_comment;
};

typedef struct
{
	uint16_t number_of_this_disk;
	uint16_t number_of_disk_with_start_of_central_directory;
	uint16_t number_of_entries_in_central_directory_on_disk;
	uint16_t total_entries_in_central_directory;
	uint32_t size_of_central_directory;
	uint32_t size_of_central_directory_offset;
	uint16_t zip_file_comment_length;
	char *zip_file_comment;
} eocd_record;

int main(void)
{
	char *buffer = NULL;
	FILE *fp;
	unsigned char c;
	size_t file_size;

	fp = fopen("out.zip", "rb");

	// If the file doesn't exist
	if (fp == NULL)
	{
		printf("Error: %d\n", errno);
		return 0;
	}

	// Go to the end of the file to get the size
	if (fseek(fp, 0L, SEEK_END) == 0)
	{
		long bufsize = ftell(fp);

		// Error
		if (bufsize == -1)
		{
			printf("There's an error here.\n");
		}

		// Allocate buffer to that size
		buffer = malloc(sizeof(char) * (bufsize + 1));

		// Go back to the start of the file
		if (fseek(fp, 0L, SEEK_SET) != 0)
		{
			printf("There's an error here.\n");
		}

		// Read the file into memory
		size_t newlen = fread(buffer, sizeof(char), bufsize, fp);
		file_size = newlen + 1;
		if (ferror(fp) != 0)
		{
			fputs("Error reading file", stderr);
		}
		else
		{
			buffer[newlen++] = '\0';
		}
		printf("Size of file: %ld\n", bufsize);
	}

	// Verify the file header
	if (buffer[0] != 0x50 || buffer[1] != 0x4b)
	{
		fputs("Error: file header does not match zip.", stderr);
	}

	eocd_record eocd;
	uint8_t *eocd_buffer;
	char *eocd_zip_file_comment_buffer;

	// Find EOCD signature
	for (size_t offset = file_size - sizeof(char) * 4; offset > 0; offset--)
	{
		// 4-byte word
		char signature[4];

		memcpy(signature, buffer + offset, sizeof(char) * 4);

		// Data is stored in little-endian format
		if (signature[0] == 0x50 && signature[1] == 0x4b && signature[2] == 0x05 && signature[3] == 0x06)
		{

			// Copy the rest of the file into the buffer.
			size_t remaining_bytes = file_size - offset - 5;

			eocd_buffer = malloc(sizeof(char) * remaining_bytes);
			memcpy(eocd_buffer, buffer + offset + 4, sizeof(char) * remaining_bytes);

			// First 2 bytes
			eocd.number_of_this_disk = eocd_buffer[0] | eocd_buffer[1] << 8;

			eocd.number_of_disk_with_start_of_central_directory = eocd_buffer[2] | eocd_buffer[3] << 8;

			eocd.number_of_entries_in_central_directory_on_disk = eocd_buffer[4] | eocd_buffer[5] << 8;

			eocd.total_entries_in_central_directory = eocd_buffer[6] | eocd_buffer[7] << 8;

			eocd.size_of_central_directory = (eocd_buffer[11] << 24) |
											 (eocd_buffer[10] << 16) |
											 (eocd_buffer[9] << 8) |
											 (eocd_buffer[8]);

			eocd.size_of_central_directory_offset = (eocd_buffer[15] << 24) |
											 (eocd_buffer[14] << 16) |
											 (eocd_buffer[13] << 8) |
											 (eocd_buffer[12]);

			eocd.zip_file_comment_length = eocd_buffer[16] | eocd_buffer[17] << 8;

			eocd_zip_file_comment_buffer = malloc(sizeof(char) * eocd.zip_file_comment_length);

			// Read the rest of the comment into the buffer
			memcpy(eocd_zip_file_comment_buffer, eocd_buffer + 18, sizeof(char) * eocd.zip_file_comment_length);
			break;
		}
	}

	printf("===== EOCD STRUCT =====\n");
	//       end of central dir signature    4 bytes  (0x06054b50)
    //   number of this disk             2 bytes
	printf("number of this disk %hu\n", eocd.number_of_this_disk);
    //   number of the disk with the
    //   start of the central directory  2 bytes
	printf("number of the disk with the start of the central directory %hu\n", eocd.number_of_disk_with_start_of_central_directory);
    //   total number of entries in the
    //   central directory on this disk  2 bytes
	printf("total number of entries in the central directory on this disk %hu\n", eocd.number_of_entries_in_central_directory_on_disk);
    //   total number of entries in
    //   the central directory           2 bytes
	printf("total number of entries in the central directory %hu\n", eocd.total_entries_in_central_directory);
    //   size of the central directory   4 bytes
	printf("size of the central directory %u\n", eocd.size_of_central_directory);
    //   offset of start of central
    //   directory with respect to
    //   the starting disk number        4 bytes
	printf("offset of start of central directory with respect to the starting disk number %u\n", eocd.size_of_central_directory_offset);
    //   .ZIP file comment length        2 bytes
	printf(".ZIP file comment length %hu\n", eocd.zip_file_comment_length);
    //   .ZIP file comment       (variable size)

	// Read central directory record into a struct

	fclose(fp);
	free(buffer);
	free(eocd_buffer);
}
