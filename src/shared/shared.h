#include <stdint.h>

struct framebuffer_info {
    void   *base;
    uint32_t  width;
    uint32_t  height;
    uint32_t  pixels_per_scanline;
    uint32_t  pitch;
};
