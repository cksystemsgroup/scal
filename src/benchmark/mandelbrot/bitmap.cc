#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "benchmark/mandelbrot/bitmap.h"
#include "benchmark/mandelbrot/rgba.h"

struct bitmap * 
bitmap_create (int w, int h) {
    struct bitmap *m;
    
    m = (struct bitmap*) malloc(sizeof(struct bitmap));
    if(!m) {
        perror ("malloc bitmap");
        return NULL;
    }
    m->data = (int*) malloc(w * h * sizeof(int));
    if(!m->data) {
        free(m);
        perror ("malloc bitmap data");
        return NULL;
    }
    m->width = w;
    m->height = h;
	return m;
}

void
bitmap_delete (struct bitmap *m) {
    free(m->data);
    free(m);
}

void 
bitmap_reset (struct bitmap *m, int value) {
    int i;
    for(i = 0; i < (m->width * m->height); i++) {
        m->data[i] = value;
    }
}

int
bitmap_get (struct bitmap *m, int x, int y) {
    x %= m->width;
    y %= m->height;
	return m->data[y * m->width + x];
}

void
bitmap_set (struct bitmap *m, int x, int y, int value ) {
    x %= m->width;
    y %= m->height;
	m->data[y * m->width + x] = value;
}

int
bitmap_width (struct bitmap *m) {
	return m->width;
}

int
bitmap_height (struct bitmap *m) {
	return m->height;
}

int *
bitmap_data (struct bitmap *m) {
	return m->data;
}

#define DIB_HDR_SIZE (40)
#define BMP_HDR_SIZE (14 + 40)

struct dib_header {
    uint32_t infosize;
    uint32_t width;
    uint32_t height;
    uint16_t planes;
    uint16_t bits;
    uint32_t compression;
    uint32_t imagesize;
    uint32_t x_res;
    uint32_t y_res;
    uint32_t nr_colors;
    uint32_t i_colors;
} __attribute__ (( packed ));

struct bmp_header {
    char magic1;
    char magic2;
	uint32_t size;
    uint16_t reserved;
    uint16_t reserved2;
    uint32_t offset;
    struct dib_header dib;
} __attribute__ (( packed ));

int
bitmap_save (struct bitmap *m, const char *path) {
    FILE *file;
    struct bmp_header header;
    int i, j;
    unsigned char *scanline, *s;

    file = fopen (path,"wb");
    if(!file) {
        perror ("bitmape file open");
        return 0;
    }

    memset (&header, 0, sizeof (header));
    header.magic1 = 'B';
    header.magic2 = 'M';
    header.size = m->width * m->height * 3 + BMP_HDR_SIZE;
    header.offset = sizeof (header);
    header.dib.infosize = 40;
    header.dib.width = m->width;
    header.dib.height = m->height;
    header.dib.planes = 1;
    header.dib.bits = 24; /* 3 x 8bit */
    header.dib.compression = 0; /* BI_RGB */
    header.dib.imagesize = m->width * m->height * 3;
    header.dib.x_res = 1000;
    header.dib.y_res = 1000;

    fwrite (&header, 1, sizeof (header), file);

	int padlength = 4 - (m->width * 3) % 4; /* lines must be a multiple of 4 bytes */
    if (padlength == 4) {
        padlength = 0;
    }

	scanline = (unsigned char*) calloc(header.dib.width * 3, sizeof(unsigned char));
    if (!scanline) {
        perror ("malloc bitmap line"); 
        fclose (file);
        return 0;
    }

    int rgba;
    for(j = 0; j < m->height; j++) {
        s = scanline;
        for(i = 0; i < m->width; i++) {
			rgba = bitmap_get (m, i, j);
			*s++ = rgba_get_blue (rgba);
			*s++ = rgba_get_green (rgba);
			*s++ = rgba_get_red (rgba);
		}
		fwrite (scanline, 1, m->width * 3, file);
		fwrite (scanline, 1, padlength, file);
	}
    //header.size += m->height * padlength;
    free (scanline);
    fclose (file);
    return 1;
}

struct bitmap *
bitmap_load (const char *path) {
    FILE *file;
    int size;
    struct bitmap *m;
    struct bmp_header header;
    int i;
    size_t read;

    file = fopen(path,"rb");
    if (!file) {
        perror ("fopen bitmap image");
        return NULL;
    }
    read = fread (&header, 1, sizeof (header), file);
    if (read != sizeof (header)) {
        fprintf (stderr, "unable to read fileheader from file");
        return NULL;
    }

    if (header.magic1 != 'B' || header.magic2 != 'M') {
        printf ("%s: %s is not a BMP file.\n", __func__, path);
        fclose (file);
        return NULL;
	}

    if (header.dib.compression != 0 || header.dib.bits != 24) {
        printf ("%s: only uncompressed 24bit bitmaps are supported.\n", __func__);
        fclose (file);
        return 0;
    }

    m = bitmap_create (header.dib.width, header.dib.height);
    if (!m) {
        fclose (file);
        return 0;
    }

    size = header.dib.width * header.dib.height;
	for(i = 0; i < size; i++) {
		int r,g,b;
		b = fgetc (file);
		g = fgetc (file);
		r = fgetc (file);
		if (b==0 && g==0 && r==0) {
			m->data[i] = 0;
		} else {
			m->data[i] = rgba_create (r, g, b, 255);
		}	
	}
	fclose (file);
	return m;
}
