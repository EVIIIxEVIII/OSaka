#pragma once
#include <stdint.h>
#include "shared/types.h"

#define PACK __attribute__((packed))

typedef struct PACK {
    void  *base;
    u32   width;
    u32   height;
    u32   pixels_per_scanline;
    u32   pitch;
} FramebufferInfo;

typedef struct PACK {
    char signature[8];
    u8   checksum;
    char oemid[6];
    u8   revision;
    u32  rsdt_address; // legacy

    u32  length;
    u64  xsdt_address;
    u8   extended_checksum;
    u8   reserved[3];
} RSDP;

typedef struct PACK {
    FramebufferInfo fb;
    RSDP* rsdp;
} BootData;
