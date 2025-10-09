#pragma once
#include <stdint.h>


typedef struct __attribute__((packed)) {
    void   *base;
    uint32_t  width;
    uint32_t  height;
    uint32_t  pixels_per_scanline;
    uint32_t  pitch;
} framebuffer_info;

typedef struct __attribute__((packed)) {
    uint64_t rsdp;
    uint64_t xsdt;
    uint64_t madt;
} acpi;

typedef struct __attribute__((packed)) {
    framebuffer_info* fb;
    acpi* acpi_data;
} uefi_data;
