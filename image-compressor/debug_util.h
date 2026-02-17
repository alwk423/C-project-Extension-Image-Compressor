#define assertFalse(x) assert(!(x))

#define panic(reason) fprintf(stderr, reason);fprintf(stderr,"\n"); assert(false)

#define panic_if(condition, reason) if (condition) {panic(reason);}

