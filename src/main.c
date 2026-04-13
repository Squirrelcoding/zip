#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

int main(void) {
	char *source = NULL;
	FILE *fp;
	unsigned char c;

	fp = fopen("example.zip", "rb");

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


  // Verify that the first few bytes match the file number

  for (int i = 0; i < 20; i++) {
    printf("%x ", source[i]);
  }

	fclose(fp);
  free(source);
}
