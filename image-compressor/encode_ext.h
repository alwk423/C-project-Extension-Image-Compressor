// encode single chunk
extern void encode_chunk(Chunk *chunk, FILE *out);
extern void encode(Chunk *ihdr, ChunkList *idats, Chunk *iend, FILE *out);
