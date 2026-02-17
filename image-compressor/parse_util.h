#define WHITESPACE " \t"
#define skip(ptr, chrs) ptr += strspn(ptr, chrs)

#define skip_ws(ptr) skip(ptr, WHITESPACE)
#define is(str, target) strncmp(str, target, strlen(target)) == 0

