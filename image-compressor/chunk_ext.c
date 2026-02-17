#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "extract_ext.h"
#include "chunk_ext.h"

#define GREY_COLOR_TYPE 0
#define FULL_COLOR_TYPE 2

#define BINARY_BIT_WIDTH 1
#define DEFAULT_BIT_WIDTH 8

// ref: https://www.w3.org/TR/png-3/#D-CRCAppendix
unsigned long crc_table[256];
int crc_table_computed = 0;

void make_crc_table(void)
{
    unsigned long c;
    int n, k;
    for (n = 0; n < 256; n++) {
        c = (unsigned long)n;
        for (k = 0; k < 8; k++) {
            if (c & 1)
                c = 0xedb88320L ^ (c >> 1);
            else
                c = c >> 1;
        }
        crc_table[n] = c;
    }
    crc_table_computed = 1;
}

unsigned long update_crc(unsigned long crc, unsigned char *buf, int len)
{
    unsigned long c = crc;
    int n;
    if (!crc_table_computed)
        make_crc_table();
    for (n = 0; n < len; n++) {
        c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
    }
    return c;
}

unsigned long crc(unsigned char *buf, int len)
{
    return update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
}


// Max size for one IDAT chunk’s data field 2³¹−1 bytes
#define MAX_IDAT_DATA 8192 

Chunk* create_chunk(const char type[5], const uint8_t* data, uint32_t length) {
    // allocate and initialize chunk structure
    Chunk* ck = malloc(sizeof(Chunk));
    assert(ck != NULL);

    // set length and type
    ck->length = length;
    memcpy(ck->type, type, 4);
    ck->type[4] = '\0';

    //set data content
    if (length) {
        ck->data = malloc(length);
        assert(ck->data != NULL);
        memcpy(ck->data, data, length);
    } else {
        ck->data = NULL;
    }

    // Build buffer for CRC, type + data
    uint8_t* buf = malloc(4 + length);
    assert(buf != NULL);
    memcpy(buf, type, 4);
    if (length) memcpy(buf + 4, data, length);

    // Compute CRC-32 (type + data)
    ck->crc = crc(buf, 4 + length);
    free(buf);

    return ck;
}


// compressed data content turned into idat chunk
ChunkList* chunk_idat(void* compressed, int length) {
    uint8_t* data = (uint8_t*)compressed; // for index 

    int max_chunks = (length + MAX_IDAT_DATA - 1) / MAX_IDAT_DATA; // max no of IDAT chunks
    Chunk** chunks = malloc(max_chunks * sizeof(Chunk*));
    assert(chunks != NULL); 

    int offset = 0; // current offset in the data
    int count = 0; // number of IDAT chunks created

    while (offset < length) {
        int sz = length - offset;
        if (sz > MAX_IDAT_DATA) sz = MAX_IDAT_DATA;
        Chunk* idat = create_chunk("IDAT", data + offset, sz);

        if (!idat) {
            for (int i = 0; i < count; i++) free_chunk(chunks[i]);
            free(chunks);
            return NULL;
        }

        // add the new chunk 
        chunks[count++] = idat;
        offset += sz;
    }

    ChunkList* list = malloc(sizeof(ChunkList));
    list->chunks = chunks;
    list->count = count;
    return list;
}

void free_chunk(Chunk* ck) {
    if (!ck) return;
    free(ck->data);
    free(ck);
}

void free_chunk_list(ChunkList* list) {
    if (!list) return;
    for (int i = 0; i < list->count; i++) {
        free_chunk(list->chunks[i]);
    }
    free(list->chunks);
    free(list);
}

Chunk* chunk_iend(void) {
    return create_chunk("IEND", NULL, 0);
}

//first chunk in png strema 
Chunk* create_ihdr(uint32_t width, uint32_t height,
                   uint8_t bit_depth, uint8_t color_type,
                   uint8_t compression, uint8_t filter,
                   uint8_t interlace) {
    uint8_t buf[13]; // data has 13 bytes , msb to lsb order
    buf[0] = width >> 24; //byte 3
    buf[1] = width >> 16; // b2 
    buf[2] = width >> 8;  // b1
    buf[3] = width; // b0
    buf[4] = height >> 24; 
    buf[5] = height >> 16;
    buf[6] = height >> 8;  
    buf[7] = height;
    buf[8] = bit_depth;
    buf[9] = color_type;
    buf[10] = compression;
    buf[11] = filter;
    buf[12] = interlace;

    return create_chunk("IHDR", buf, 13);
}

Chunk* chunk_ihdr(uint32_t width, uint32_t height, Format format)
{
    uint8_t compression = 0;
    uint8_t filter = 0;
    uint8_t interlace = 0;

    uint8_t bit_depth;
    uint8_t color_type;                
    switch(format){
        case BW:
            color_type = GREY_COLOR_TYPE;
            bit_depth = BINARY_BIT_WIDTH;
            break;
        
        case GREYSCALE:
            color_type = GREY_COLOR_TYPE;
            bit_depth = DEFAULT_BIT_WIDTH;
            break;
        
        case FULL_COLOR:
            color_type = FULL_COLOR_TYPE;
            bit_depth = DEFAULT_BIT_WIDTH;
            break;

        default:
            // not valid format
            assert(false);
    }

    return create_ihdr(width, height, bit_depth, color_type, compression, filter, interlace);

}
