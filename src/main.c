#include <stddef.h>
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
	char *file_name;
	char *extra_field;
} local_file_header;

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

char *read_string(uint8_t *source, size_t offset, size_t length)
{
	char *ptr = malloc(sizeof(char) * length);

	memcpy(ptr, source + offset, sizeof(char) * length);

	return ptr;
}

size_t read_bits(uint8_t *source, size_t start, size_t size)
{
  size_t output = 0;

  for (size_t i = 0; i < size; i++)
    output |= source[start + i] << 8 * i;

  return output;
}

int main(void)
{
	uint8_t *buffer = NULL;
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

			eocd.number_of_this_disk = read_bits(eocd_buffer, 0, 2);
			eocd.number_of_disk_with_start_of_central_directory = read_bits(eocd_buffer, 2, 2);
			eocd.number_of_entries_in_central_directory_on_disk = read_bits(eocd_buffer, 4, 2);
			eocd.total_entries_in_central_directory = read_bits(eocd_buffer, 6, 2);
			eocd.size_of_central_directory = read_bits(eocd_buffer, 8, 4);
			eocd.size_of_central_directory_offset = read_bits(eocd_buffer, 12, 4);
			eocd.zip_file_comment_length = read_bits(eocd_buffer, 16, 2);
			eocd.zip_file_comment = read_string(eocd_buffer, 18, eocd.zip_file_comment_length);

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

		temp_cd_file_header.version_made_by = read_bits(buffer, idx, 2);
		temp_cd_file_header.version_needed_to_extract = read_bits(buffer, idx + 2, 2);
		temp_cd_file_header.general_purpose_bit_flag = read_bits(buffer, idx + 4, 2);
		temp_cd_file_header.compression_method = read_bits(buffer, idx + 6, 2);
		temp_cd_file_header.last_mod_file_time = read_bits(buffer, idx + 8, 2);
		temp_cd_file_header.last_mod_file_date = read_bits(buffer, idx + 10, 2);
		temp_cd_file_header.crc_32 = read_bits(buffer, idx + 12, 4);
		temp_cd_file_header.compressed_size = read_bits(buffer, idx + 16, 4);
		temp_cd_file_header.uncompressed_size = read_bits(buffer, idx + 20, 4);
		temp_cd_file_header.file_name_length = read_bits(buffer, idx + 24, 2);
		temp_cd_file_header.extra_field_length = read_bits(buffer, idx + 26, 2);
		temp_cd_file_header.file_comment_length = read_bits(buffer, idx + 28, 2);
		temp_cd_file_header.disk_number_start = read_bits(buffer, idx + 30, 2);
		temp_cd_file_header.internal_file_attributes = read_bits(buffer, idx + 32, 2);
		temp_cd_file_header.external_file_attributes = read_bits(buffer, idx + 34, 4);
		temp_cd_file_header.relative_offset_of_local_header = read_bits(buffer, idx + 38, 4);

		temp_cd_file_header.file_name = read_string(buffer, idx + 42, temp_cd_file_header.file_name_length);
		temp_cd_file_header.extra_field = read_string(buffer, idx + 42 + temp_cd_file_header.file_name_length, temp_cd_file_header.extra_field_length);
		temp_cd_file_header.file_comment = read_string(
			buffer, idx + 42 + temp_cd_file_header.file_name_length + temp_cd_file_header.extra_field_length,
			temp_cd_file_header.file_comment_length);

		// Move this pointer to the start of the next file header
		start_of_header += 46 + temp_cd_file_header.file_name_length + temp_cd_file_header.extra_field_length + temp_cd_file_header.file_comment_length;

		// Add the current file header to the list of file headers
		cd_file_headers[i] = temp_cd_file_header;
	}

	// Loops through all the files to unzip them
	for (size_t i = 0; i < eocd.number_of_entries_in_central_directory_on_disk; i++) {

		local_file_header temp_header;

		printf("Compression method: %u\n", cd_file_headers[i].compression_method);

		printf("%s\n", cd_file_headers[i].file_name);

		// Start of the file header, skipping the header.
		size_t p = cd_file_headers[i].relative_offset_of_local_header + 4;

		temp_header.version_needed_to_extract = read_bits(buffer, p, 2);
		temp_header.general_purpose_bit_flag = read_bits(buffer, p + 2, 2);
		temp_header.compression_method = read_bits(buffer, p + 4, 2);
		temp_header.last_mod_file_time = read_bits(buffer, p + 6, 2);
		temp_header.last_mod_file_date = read_bits(buffer, p + 8, 2);
		temp_header.crc_32 = read_bits(buffer, p + 10, 4);
		temp_header.compressed_size = read_bits(buffer, p + 14, 4);
		temp_header.uncompressed_size = read_bits(buffer, p + 18, 4);
		temp_header.file_name_length = read_bits(buffer, p + 22, 2);
		temp_header.extra_field_length = read_bits(buffer, p + 24, 2);
		temp_header.file_name = read_string(buffer, p + 26, temp_header.file_name_length);
		temp_header.extra_field = read_string(buffer, p + 26 + temp_header.file_name_length, temp_header.extra_field_length);

		printf("Ccompressed size: %u\n", temp_header.compressed_size);
		printf("Uncompressed size: %u\n", temp_header.uncompressed_size);
		printf("File name length: %s\n", temp_header.file_name);

		char *compressed_data = malloc(sizeof(char) * temp_header.compressed_size);

		p += 26 + temp_header.file_name_length + temp_header.extra_field_length;
		memcpy(compressed_data, buffer + p, sizeof(char) * temp_header.compressed_size);

		// Write the file data to a file
		FILE *output;
		fp = fopen(temp_header.file_name, "wb");

		fwrite(compressed_data, sizeof(char), temp_header.compressed_size, fp);

		fclose(fp);
		free(temp_header.file_name);
		free(temp_header.extra_field);
		free(compressed_data);
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
