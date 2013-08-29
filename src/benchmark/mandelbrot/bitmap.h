#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <inttypes.h>

struct bitmap {
    int width;
    int height;
    int *data;
};

#ifdef __cplusplus
extern "C" {
#endif /* __CPLUSPLUS */

struct bitmap * bitmap_create (int, int);
void bitmap_delete (struct bitmap *);
struct bitmap * bitmap_load (const char *);
int bitmap_save (struct bitmap *, const char *);

int bitmap_get (struct bitmap *, int, int);
void bitmap_set (struct bitmap *, int, int ,int);
int bitmap_width (struct bitmap *);
int bitmap_height (struct bitmap *);
void bitmap_reset (struct bitmap *, int);
int * bitmap_data (struct bitmap *);

#ifdef __cplusplus
}
#endif /* __CPLUSPLUS */

#endif /* __BITMAP_H__ */
