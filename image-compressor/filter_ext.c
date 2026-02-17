#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <limits.h>

#include "extract_ext.h"
#include "filter_ext.h"

#define BINARY_PIXEL_SIZE 1
#define GREY_PIXEL_SIZE 1
#define COLOR_PIXEL_SIZE 3

static int sub_filter(uint8_t x, uint8_t a) {
    return (x - a) & 0xFF;
}

static int up_filter(uint8_t x, uint8_t b) {
    return (x - b) & 0xFF;
}

static int average_filter(uint8_t x, uint8_t a, uint8_t b) {
    return (x - (a + b) / 2) & 0xFF;
}

static int paeth_predict(uint8_t a, uint8_t b, uint8_t c) {
    int res;
    int p = a + b - c;
    int pa = abs(p - a);
    int pb = abs(p - b);
    int pc = abs(p - c);
    if (pa <= pb && pa <= pc) {
        res = a;
    } else if (pb <= pc) {
        res = b;
    } else {
        res = c;
    }
    return res;
}


static int min_diff(int filtered_line[5]) {
    int ind;
    int min_v = INT_MAX;
    for (int i = 0; i < 5;i++) {
        if (filtered_line[i] < min_v) {
            min_v = filtered_line[i];
            ind = i;
        }
    }
    return ind;
}

// row_length and num_row from image width and height
uint8_t ** filter(uint8_t ** scanlines, int row_length, int num_row, Format format) {
    int bpp; // bits per pixel

    switch (format) {
        case BW:
            bpp = BINARY_PIXEL_SIZE;
            break;
        case GREYSCALE:
            bpp = GREY_PIXEL_SIZE;
            break;
        case FULL_COLOR:
            bpp = COLOR_PIXEL_SIZE;
            break;
        default:
            fprintf(stderr, "Unsupported format\n");
            exit(EXIT_FAILURE);
    }

    // initialise return array 
    uint8_t ** lines = malloc(num_row * sizeof(uint8_t *));
    // iterate over each scanline
    for (int r = 0; r < num_row; r++) {
        uint8_t * line0 = malloc((row_length + 1) * sizeof(uint8_t));
        uint8_t * line1 = malloc((row_length + 1) * sizeof(uint8_t));
        uint8_t * line2 = malloc((row_length + 1) * sizeof(uint8_t));
        uint8_t * line3 = malloc((row_length + 1) * sizeof(uint8_t));
        uint8_t * line4 = malloc((row_length + 1) * sizeof(uint8_t));
        line0[0] = 0;
        line1[0] = 1;
        line2[0] = 2;
        line3[0] = 3;
        line4[0] = 4;
        int type0 = 0;
        int type1 = 0;
        int type2 = 0;
        int type3 = 0;
        int type4 = 0;
        // iterate over each byte in each scanline
        for (int l = 0; l < row_length; l++) {
            uint8_t c = ((r - 1 < 0) || (l - bpp < 0)) ? 0 : scanlines[r - 1][l - bpp];
            uint8_t b = (r - 1 < 0) ? 0 : scanlines[r - 1][l];
            uint8_t a = (l - bpp < 0) ? 0 : scanlines[r][l - bpp];
            uint8_t x = scanlines[r][l];

            line0[l + 1] = (uint8_t)x;
            line1[l + 1] = (uint8_t)sub_filter(x, a);
            line2[l + 1] = (uint8_t)up_filter(x, b);
            line3[l + 1] = (uint8_t)average_filter(x, a, b);
            line4[l + 1] = (uint8_t)((x -paeth_predict(a, b , c)) & 0xFF);

            type0 += x;
            type1 += abs(sub_filter(x, a));
            type2 += abs(up_filter(x, b));
            type3 += abs(average_filter(x, a, b));
            type4 += abs((x -paeth_predict(a, b , c)) & 0xFF);
        }

        int min_ind = min_diff((int[5]){type0, type1, type2, type3, type4});
        switch (min_ind)
        {
        case 0:
            lines[r] = line0;
            free(line1);
            free(line2);
            free(line3);
            free(line4);
            break;
        case 1:
            lines[r] = line1;
            free(line0);
            free(line2);
            free(line3);
            free(line4);
            break;
        case 2:
            lines[r] = line2;
            free(line0);
            free(line1);
            free(line3);
            free(line4);
            break;
        case 3:
            lines[r] = line3;
            free(line0);
            free(line1);
            free(line2);
            free(line4);
            break;
        case 4:
            lines[r] = line4;
            free(line0);
            free(line1);
            free(line2);
            free(line3);
            break;
        default:
            break;
        }
    }
    return lines;
}



