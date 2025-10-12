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
    char Signature[8];
    uint8_t Checksum;
    char OEMID[6];
    uint8_t Revision;
    uint32_t RsdtAddress; // legacy

    uint32_t Length;
    uint64_t XsdtAddress;
    uint8_t ExtendedChecksum;
    uint8_t reserved[3];
} RSDP;

typedef struct PACK {
    FramebufferInfo fb;
    RSDP* rsdp;
} BootData;
