#include <stdio.h>
#include <stdlib.h> 
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <arpa/inet.h>

#include "extract_ext.h"
#include "chunk_ext.h"

void encode_chunk(Chunk *chunk, FILE *out){
  assert(chunk != NULL);
  assert(out != NULL);

  // convert length and crc to MSB
  uint32_t length = htonl(chunk->length);
  uint32_t crc = htonl(chunk->crc);

  // write length (4 bytes)
  fwrite(&length, sizeof(uint32_t), 1, out);

  //write type (4 bytes)
  fwrite(chunk->type, sizeof(char), 4, out);

  // write data
  // check chunk is not empty
  if (chunk->length > 0){
    fwrite(chunk->data, sizeof(uint8_t), chunk->length, out);
  }

  //write crc (4 bytes)
  fwrite(&crc, sizeof(uint32_t), 1, out);
}


void encode(Chunk *ihdr, ChunkList *idats, Chunk *iend, FILE *out){
  assert(ihdr != NULL);
  assert(idats != NULL);
  assert(iend != NULL);
  assert(out != NULL);

  // PNG signature at first 8 bytes
  uint8_t signature[8] = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A
  };
  fwrite(signature, 1, 8, out);

  // encode IHDR chunk
  encode_chunk(ihdr, out);

  // encode IDAT chunks
  for (int i = 0; i < idats->count; i++){
    encode_chunk(idats->chunks[i], out);
  }

  //encode IEND chunk
  encode_chunk(iend, out);
}
