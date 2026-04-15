#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

typedef struct
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
} zip_header;

typedef struct
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
	char *file_name;
	char *extra_field;
	char *file_comment;
} cd_file_header;

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

char *read_string_u8(uint8_t *source, size_t offset, size_t length)
{
	char *ptr = malloc(sizeof(char) * length);

	memcpy(ptr, source + offset, sizeof(char) * length);

	return ptr;
}

char *read_string_char(char *source, size_t offset, size_t length)
{
	char *ptr = malloc(sizeof(char) * length);

	memcpy(ptr, source + offset, sizeof(char) * length);

	return ptr;
}

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

			eocd.zip_file_comment = read_string_u8(eocd_buffer, 18, eocd.zip_file_comment_length);
			break;
		}
	}

	// Array to store central directory file headers
	cd_file_header *cd_file_headers = (cd_file_header *)malloc(sizeof(cd_file_header) * eocd.number_of_entries_in_central_directory_on_disk);
	cd_file_header temp_cd_file_header;

	size_t start_of_header = eocd.size_of_central_directory_offset;

	// Loop to keep finding local file headers
	for (size_t i = 0; i < eocd.number_of_entries_in_central_directory_on_disk; i++)
	{
		// Index for where the data actually starts; we add 4 to skip the magic number.
		size_t idx = start_of_header + 4;

		temp_cd_file_header.version_made_by = buffer[idx] | buffer[idx + 1] << 8;
		temp_cd_file_header.version_needed_to_extract = buffer[idx + 2] | buffer[idx + 3] << 8;
		temp_cd_file_header.general_purpose_bit_flag = buffer[idx + 4] | buffer[idx + 5] << 8;
		temp_cd_file_header.compression_method = buffer[idx + 6] | buffer[idx + 7] << 8;
		temp_cd_file_header.last_mod_file_time = buffer[idx + 8] | buffer[idx + 9] << 8;
		temp_cd_file_header.last_mod_file_date = buffer[idx + 10] | buffer[idx + 11] << 8;

		temp_cd_file_header.crc_32 = buffer[idx + 12] |
									 buffer[idx + 13] << 8 |
									 buffer[idx + 14] << 16 |
									 buffer[idx + 15] << 24;

		temp_cd_file_header.compressed_size = buffer[idx + 16] |
											  buffer[idx + 17] << 8 |
											  buffer[idx + 18] << 16 |
											  buffer[idx + 19] << 24;

		temp_cd_file_header.uncompressed_size = buffer[idx + 20] |
												buffer[idx + 21] << 8 |
												buffer[idx + 22] << 16 |
												buffer[idx + 23] << 24;

		temp_cd_file_header.file_name_length = buffer[idx + 24] | buffer[idx + 25] << 8;
		temp_cd_file_header.extra_field_length = buffer[idx + 26] | buffer[idx + 27] << 8;
		temp_cd_file_header.file_comment_length = buffer[idx + 28] | buffer[idx + 29] << 8;
		temp_cd_file_header.disk_number_start = buffer[idx + 30] | buffer[idx + 31] << 8;
		temp_cd_file_header.internal_file_attributes = buffer[idx + 32] | buffer[idx + 33] << 8;

		temp_cd_file_header.external_file_attributes = buffer[idx + 34] |
													   buffer[idx + 35] << 8 |
													   buffer[idx + 36] << 16 |
													   buffer[idx + 37] << 24;

		temp_cd_file_header.relative_offset_of_local_header = buffer[idx + 38] |
															  buffer[idx + 39] << 8 |
															  buffer[idx + 40] << 16 |
															  buffer[idx + 41] << 24;

		// Read file name
		temp_cd_file_header.file_name = read_string_char(buffer, idx + 42, temp_cd_file_header.file_name_length);
		// Read extra field
		temp_cd_file_header.extra_field = read_string_char(buffer, idx + 42 + temp_cd_file_header.file_name_length, temp_cd_file_header.extra_field_length);
		// Read file comment
		temp_cd_file_header.file_comment = read_string_char(
			buffer, idx + 42 + temp_cd_file_header.file_name_length + temp_cd_file_header.extra_field_length,
			temp_cd_file_header.file_comment_length);

		// Move this pointer to the start of the next file header
		start_of_header += 46 + temp_cd_file_header.file_name_length + temp_cd_file_header.extra_field_length + temp_cd_file_header.file_comment_length;

		// Add the current file header to the list of file headers
		cd_file_headers[i] = temp_cd_file_header;
	}

	

	free(temp_cd_file_header.file_name);
	free(temp_cd_file_header.extra_field);
	free(temp_cd_file_header.file_comment);

	fclose(fp);
	free(buffer);
	free(eocd.zip_file_comment);
	free(eocd_buffer);
	free(cd_file_headers);
}
