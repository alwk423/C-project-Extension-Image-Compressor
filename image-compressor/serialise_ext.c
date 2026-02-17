#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h> 
#include <assert.h>

#include "extract_ext.h"

uint8_t** serialise(void *buffer, int width, int height, Format format, int *length){
	Pixel *pixels = buffer;

	int row_len;
	switch(format){
		case BW:
			// 1 bit per pixel, 8 pixels per byte
			// ensures extra bits are packed into a byte
			row_len = (width + 7) / 8;
			break;

		case GREYSCALE:
			// // 1 byte per pixel (8-bit grayscale)
			row_len = width;
			break;

		case FULL_COLOR:
			// 3 bytes per pixel (R, G, B)
			row_len = width * 3;
			break;

		default:
			// not valid format
			assert(false);
	}

	// malloc memory
	uint8_t **out = malloc(height * sizeof(uint8_t*));
	assert(out != NULL);
	for (int i = 0; i < height; i++){
		if (format == BW){
			out[i] = calloc(row_len, sizeof(uint8_t));
		} else{
			out[i] = malloc(row_len * sizeof(uint8_t));
		}
		assert(out[i] != NULL);
	}

	for (int row = 0; row < height; row++){
		for (int col = 0; col < width; col++){
			// pixels is an 1D array, calculate index for corresponding row and col
			int index = row * width + col;

			switch(format){
				case BW:
					// set 0 if pixel is black
					if(!pixels[index].bw){
						int byte_no = col / 8;
						// MSB
						int bits_offset = 7 - (col % 8);
						// set 1 at correct position
						out[row][byte_no] |= (1 << bits_offset);
					}
					break;

				case GREYSCALE:
					// Store 8 bits(one byte) grayscale value directly
					out[row][col] = pixels[index].gp;
					break;

				case FULL_COLOR:
					{
							// each pixel is 3 bytes
							int offset = 3 * col;
							// store red
							out[row][offset] = (uint8_t)pixels[index].cp.red;
							// store green
							out[row][offset + 1] = (uint8_t)pixels[index].cp.green;
							// store blue
							out[row][offset + 2] = (uint8_t)pixels[index].cp.blue;
					}
					break;

				default:
					// not valid format
					assert(false);
			}
		}
	}

	*length = row_len;
	return out;
}
