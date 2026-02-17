#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/param.h>
#include <zlib.h>

#include "compress_ext.h"

#include "debug_util.h"

#define CHUNK 32768

#define MIN_WINDOW_BITS 9
#define MAX_WINDOW_BITS 15

#define DEFAULT_COMPRESSION_LEVEL 6

#define DEFAULT_MEM_LEVEL 8

#define DEFLATE_COMPRESSION_CODE 8

static void decode_zcodes(int code) {
	switch (code) {
		case Z_ERRNO:
			fprintf(stderr, "Error while reading/writing from external source\n");
			break;
		case Z_STREAM_ERROR:
			fprintf(stderr, "Invalid compression level\n");
			break;
		case Z_DATA_ERROR:
			fprintf(stderr, "Invalid or incomplete deflation data\n");
			break;
		case Z_MEM_ERROR:
			fprintf(stderr, "Unable to allocate memory\n");
			break;
		case Z_VERSION_ERROR:
			fprintf(stderr, "Incompatible zlib versions\n");
			break;
	}
}

void *compress_lines(uint8_t **mlines, int mlines_len, int mline_len, int *length) {
	// referenced from https://zlib.net/zpipe.c
	int cap = mlines_len;
	int len = 0;
	uint8_t *res = malloc(sizeof(uint8_t) * cap);
	assert(res != NULL);

	// status code for zlib
	int code = Z_ERRNO;
	z_stream stream;
	uint8_t out[CHUNK];

	// init stream
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;

	int min_window_bits = MAX(MIN_WINDOW_BITS, ceil(log2(mlines_len * mline_len)));

	int window_bits = MIN(MAX_WINDOW_BITS, min_window_bits);

	code = deflateInit2(
			&stream,
			DEFAULT_COMPRESSION_LEVEL,
			DEFLATE_COMPRESSION_CODE,
			window_bits,
			DEFAULT_MEM_LEVEL,
			Z_DEFAULT_STRATEGY
		);

	if (code != Z_OK) {
		decode_zcodes(code);
		panic("Error while intiating deflation stream");
	}

	uint8_t *line = *mlines;
	for (int i = 0; i < mlines_len; i++) {
		line = mlines[i];
		// copy in mline
		stream.avail_in = mline_len;
		stream.next_in = line;

		// deflate till buffer isn't filled when deflating
		do {
			stream.avail_out = CHUNK;
			stream.next_out = out;
			code = deflate(&stream, i == (mlines_len - 1));
			int comped_bytes = CHUNK - stream.avail_out;
			if (len + comped_bytes >= cap) {
				// resize buffer
				res = realloc(res, sizeof(uint8_t) * (cap += mline_len + comped_bytes));
				assert(res != NULL);
			}
			for (int j = 0; j < comped_bytes; j++) {
				res[len++] = out[j];
			}
		} while (stream.avail_out == 0);
		panic_if (stream.avail_in != 0, "All input should be consumed before continuing");

	}

	// cleanup
	// close deflate process
	(void) deflateEnd(&stream);
	// store length of compressed bytes
	*length = len;
	return res;
}

