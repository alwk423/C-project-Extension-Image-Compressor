#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "extract_ext.h"
#include "serialise_ext.h"
#include "filter_ext.h"
#include "compress_ext.h"
#include "chunk_ext.h"
#include "encode_ext.h"


int main(int argc, char **argv) {
	assert(argc == 3);

	Format format;
	int height;
	int width;
	int scanline_width;
	void *pixels = extract(argv[1], &format, &height, &width);

	// open output file
	FILE *out = fopen(argv[2], "wb");
	if (out == NULL){
		printf("Failed to open the output fiule.");
		return 1;
	}

	uint8_t **scanlines = serialise(pixels, width, height, format, &scanline_width);

	uint8_t **mlines = filter(scanlines, scanline_width, height, format);

	int len;
	void *compressed = compress_lines(mlines, height, scanline_width + 1, &len);
	// make chunks
	Chunk *ihdr = chunk_ihdr(width, height, format);
	ChunkList* idats = chunk_idat(compressed, len);
	Chunk* iend = chunk_iend();

	// encode
	encode(ihdr, idats, iend, out);

	fclose(out);

	return 0;
}
