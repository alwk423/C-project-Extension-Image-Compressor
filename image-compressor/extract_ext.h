
typedef enum {
	BW,
	GREYSCALE,	// assume constant 8 bits depth
	FULL_COLOR	// assume constant 8 bits depth
} Format;

typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} CPixel;

typedef union {
	bool bw;
	uint8_t gp;
	CPixel cp;
} Pixel;	

extern Pixel *extract(char *source, Format *format, int *height, int *width);

