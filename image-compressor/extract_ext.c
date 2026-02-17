#define _POSIX_C_SOURCE 200809L

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <assert.h>

#include "extract_ext.h"

#include "parse_util.h"
#include "debug_util.h"

#define DEFAULT_DEPTH 255

static void extract_header(FILE *source, char *buff) {
	fgets(buff, 3, source);
}

static void extract_dimensions(FILE *source, int *height, int *width) {
	char *buff = NULL;
	size_t cap = 0;
	int len;
	while ((len = getline(&buff, &cap, source)) == 1 || *buff == '#');

	skip_ws(buff);

	char *end = NULL;
	*width = strtol(buff, &end, 10);

	if (buff == end) {
		fclose(source);
		panic("Expected digit");
	}
	buff = end;

	skip_ws(buff);

	*height = strtol(buff, &end, 10);
	if (buff == end) {
		fclose(source);
		panic("Expected digit");
	}
}

static Pixel *extract_P1(FILE *src, size_t size) {
	Pixel *pixels = malloc(sizeof(Pixel) * size);
	assert(pixels != NULL);

	int character;
	for (int i = 0; i < size; i++) {
		// skip to a 0 or 1
		while ((character = fgetc(src)) != EOF &&
				(char) character != '0' && (char) character != '1');

		if (character == EOF) {
			fclose(src);
			panic("Unexpected end of file");
		}

		pixels[i].bw = (char) character == '1';
	}
	fclose(src);
	return pixels;
}

static void decode_into_bool_array(Pixel *bools, size_t index, char target) {
	for (int i = 7; i >= 0; i--) {
		bools[index + i].bw = (target & 0x1) == 1;
		target >>= 1;
	}
}

static Pixel *extract_P4(FILE *src, size_t size) {
	Pixel *pixels = calloc(sizeof(Pixel), (size+7)/8*8);
	assert(pixels != NULL);

	int character;
	size_t i = 0;
	while (((character = fgetc(src)) != EOF)) {
		decode_into_bool_array(pixels, (i++) * 8, character);
	}
	assert(size/8 == i);
	if (ferror(src)) {
		panic("Unknown error while reading file");
	}
	fclose(src);
	return pixels;
}

static Pixel *extract_BW(FILE *src, char *magic, int height, int width) {
	if (!(is(magic, "P1") || is(magic, "P4"))) {
		fclose(src);
		panic("Expected P1 or P4 magic number");
	}

	if (is(magic, "P1")) {
		return extract_P1(src, height * width);
	} else {
		return extract_P4(src, height * width);
	}
}

static uint16_t upscale(int cur_depth, uint16_t target) {
	// int max_out_sample = pow(2, ceil(log2(cur_depth+1)))-1;
	int max_out_sample = DEFAULT_DEPTH;
	// from https://www.w3.org/TR/2003/REC-PNG-20031110/#12Sample-depth-scaling
	return floor((target * max_out_sample / cur_depth) + 0.5);
}

static Pixel *extract_P2(FILE *src, size_t size, int depth) {
	Pixel *pixels = malloc(sizeof(Pixel)*size);
	assert(pixels != NULL);
	size_t p = 0;

	uint16_t num;
	char digits[4];
	int i = 0;

	char character;
	do {
		character = fgetc(src);
		if (!isdigit(character)) {
			if (i == 0) continue;
			digits[i] = '\0';
			num = strtol(digits, NULL, 10);
			pixels[p++].gp = upscale(depth, num);
			i = 0;
		} else {
			digits[i++] = character;
		}

	} while (character != EOF);

	if (p < size) {
		fclose(src);
		panic("insufficent pixels in file");
	}

	fclose(src);
	return pixels;
}

static Pixel *extract_P5(FILE *src, size_t size, int depth) {
	Pixel *pixels = malloc(sizeof(Pixel) * size);
	assert(pixels != NULL);

	for (size_t i = 0; i < size; i++) {
		int val = fgetc(src);
		if (val == EOF) {
			fclose(src);
			panic("Unexpected end of file, insufficent pixels");
		}
		panic_if(val < 0 || val > 255, "Unexpected error when reading pixel for P5 format");
		pixels[i].gp = upscale(depth, (uint8_t) val);
	}
	fclose(src);
	return pixels;
}

static Pixel *extract_GREY(FILE *src, char *magic, int height, int width, int depth) {
	if (!(is(magic, "P2") || is(magic, "P5"))) {
		fclose(src);
		panic("Expected P2 or P4 magic number");
	}

	if (is(magic, "P2")) {
		return extract_P2(src, height * width, depth);
	} else {
		return extract_P5(src, height * width, depth);
	}
}

static Pixel *extract_P3(FILE *src, size_t size, int depth) {
	Pixel *pixels = malloc(sizeof(Pixel) * size);
	assert(pixels != NULL);
	size_t p = 0;

	uint16_t num;
	char digits[4];
	int i = 0;
	int sample = 0;

	char character;
	do {
		character = fgetc(src);
		if (!isdigit(character)) {
			if (i == 0) continue;
			digits[i] = '\0';
			num = strtol(digits, NULL, 10);
			int sample_val = upscale(depth, num);
			switch (sample++) {
				case 0:
					pixels[p].cp.red = sample_val;
					break;
				case 1:
					pixels[p].cp.green = sample_val;
					break;
				case 2:
					pixels[p].cp.blue = sample_val;
					sample = 0;
					p++;
					break;
			}
			i = 0;
		} else {
			digits[i++] = character;
		}

	} while (character != EOF);

	if (p < size) {
		fclose(src);
		panic("insufficent pixels in file");
	}

	fclose(src);
	return pixels;
}

static Pixel *extract_P6(FILE *src, size_t size, int depth) {
	Pixel *pixels = malloc(sizeof(Pixel) * size);
	assert(pixels != NULL);

	for (size_t i = 0; i < size; i++) {
		// get red sample
		int val = fgetc(src);
		if (val == EOF) {
			fclose(src);
			panic("Unexpected end of file, insufficent pixels");
		}
		panic_if(val < 0 || val > 255, "Unexpected error when reading pixel for P6 format");
		pixels[i].cp.red = upscale(depth, (uint8_t) val);

		// get green sample
		val = fgetc(src);
		if (val == EOF) {
			fclose(src);
			panic("Unexpected end of file, insufficent pixels");
		}
		panic_if(val < 0 || val > 255, "Unexpected error when reading pixel for P6 format");
		pixels[i].cp.green = upscale(depth, (uint8_t) val);

		// get blue sample
		val = fgetc(src);
		if (val == EOF) {
			fclose(src);
			panic("Unexpected end of file, insufficent pixels");
		}
		panic_if(val < 0 || val > 255, "Unexpected error when reading pixel for P6 format");
		pixels[i].cp.blue = upscale(depth, (uint8_t) val);
	}
	fclose(src);
	return pixels;
}

static Pixel *extract_color(FILE *src, char *magic, int height, int width, int depth) {
	if (!(is(magic, "P3") || is(magic, "P6"))) {
		fclose(src);
		panic("Expected P3 or P6 magic number");
	}

	if (is(magic, "P3")) {
		return extract_P3(src, height * width, depth);
	} else {
		return extract_P6(src, height * width, depth);
	}
}
Pixel *extract(char *source, Format *format, int *height, int *width) {
	char *extension = strrchr(source, '.');
	panic_if(extension == NULL, "there must be an extension given");

	FILE *src = fopen(source, "r");

	panic_if(src == NULL, "No such file found");

	char magic[3];
	magic[2] = '\0';    // fixed: terminate 2-char magic correctly

	extract_header(src, magic);

	extract_dimensions(src, height, width);

	if (is(extension, ".pbm")) {
		*format = BW;
		return extract_BW(src, magic, *height, *width);
	}

	int depth;
	// extract "bit depth"
	char *buff = NULL;
	size_t cap = 0;
	int len;
	while ((len = getline(&buff, &cap, src)) == 1 || *buff == '#');

	skip_ws(buff);

	char *end = NULL;
	depth = strtol(buff, &end, 10);

	if (buff == end) {
		fclose(src);
		panic("Expected digit when parsing for sample depth");
	}
	buff = end;

	panic_if(depth > 255 || depth < 0, "depth outside of range 0 <= depth < 256");

	if (is(extension, ".pgm")) {
		*format = GREYSCALE;
		return extract_GREY(src, magic, *height, *width, depth);
	} else if (is(extension, ".ppm")) {
		*format = FULL_COLOR;
		return extract_color(src, magic, *height, *width, depth);
	}
	panic("Invalid extension encountered");
}
