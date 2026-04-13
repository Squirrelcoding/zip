#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

struct zip_header {
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

int main(void) {
	char *source = NULL;
	FILE *fp;
	unsigned char c;

	fp = fopen("APPNOTE.txt", "rb");

	// If the file doesn't exist
	if (fp == NULL) {
		printf("Error: %d\n", errno);
		return 0;
	}

	// Go to the end of the file to get the size
	if (fseek(fp, 0L, SEEK_END) == 0) {
		long bufsize = ftell(fp);

		// Error
		if (bufsize == -1) {
      printf("There's an error here.\n");
		}

    // Allocate buffer to that size
    source = malloc(sizeof(char) * (bufsize + 1));

    // Go back to the start of the file
    if (fseek(fp, 0L, SEEK_SET) != 0) {
      printf("There's an error here.\n");
    }

    // Read the file into memory
    size_t newlen = fread(source, sizeof(char), bufsize, fp);
    if (ferror(fp) != 0) {
      fputs("Error reading file", stderr);
    } else {
      source[newlen++] = '\0';
    }
    printf("Size of file: %ld\n", bufsize);
	}


  // Verify the file header
  if (source[0] != 0x50 || source[1] != 0x4b) {
    fputs("Error reading file", stderr);
  }
  

  // TODO: Read the header into the struct.

	fclose(fp);
  free(source);
}
