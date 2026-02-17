#ifndef CHUNK_H
#define CHUNK_H
#include <stdint.h>

typedef struct {
    uint32_t length;   // data length
    char type[5];      // 4-letter type + null-terminator
    uint8_t* data;     // content of the chunk
    uint32_t crc;      // type+data
} Chunk;

typedef struct {
    Chunk** chunks;
    int count;
} ChunkList;

// Create generic chunk
extern Chunk* create_chunk(const char type[5], const uint8_t* data, uint32_t length);

// Create IHDR chunk (width, height, bit depth, color type, compression, filter, interlace)
// for header of image
extern Chunk* create_ihdr(uint32_t width, uint32_t height,
                   uint8_t bit_depth, uint8_t color_type,
                   uint8_t compression, uint8_t filter,
                   uint8_t interlace);

// get values for create_ihdr
// return the final ihdr
extern Chunk* chunk_ihdr(uint32_t width, uint32_t height, Format format);

// Create IEND chunk
// for footer of image
extern Chunk* chunk_iend(void);

// Split compressed data into IDAT chunks
extern ChunkList* chunk_idat(void* compressed, int length);

// Free chunk and its data
extern void free_chunk(Chunk* ck);

// Free list of chunks
extern void free_chunk_list(ChunkList* list);


#endif
