#pragma once
#include <stdint.h>

#define PACK __attribute__((packed))

typedef struct PAC {
    void      *base;
    uint32_t  width;
    uint32_t  height;
    uint32_t  pixels_per_scanline;
    uint32_t  pitch;
} FramebufferInfo;

typedef struct PACK {
    char signature[8];
    uint8_t checksum;
    char oemid[6];
    uint8_t revision;
    uint32_t rsdt_address; // legacy

    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} RSDP;

typedef struct PACK {
    FramebufferInfo fb;
    RSDP* rsdp;
} BootData;
