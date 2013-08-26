#ifndef __RGBA_H__
#define __RGBA_H__

#include <stdint.h>

__inline__ uint32_t
rgba_create(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return ((((int32_t)a) << 24)
            | (((int32_t)r) << 16)
            | (((int32_t)g) << 8)
            | (((int32_t)b) << 0));
}

__inline__ char
rgba_get_alpha(uint32_t rgba) {
    return ((rgba >> 24) & 0xFF);
}

__inline__ char
rgba_get_red(uint32_t rgba) {
    return ((rgba >> 16) & 0xFF);
}

__inline__ char
rgba_get_green(uint32_t rgba) {
    return ((rgba >> 8) & 0xFF);
}

__inline__ char
rgba_get_blue(uint32_t rgba) {
    return ((char)rgba & 0xFF);
}

#endif /* __RGBA_H__ */
