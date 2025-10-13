#pragma once
#include "types.h"

typedef struct PACK {
  char signature[4];
  u32 length;
  u8 revision;
  u8 checksum;
  char oem_id[6];
  char oem_table_id[8];
  u32 oem_revision;
  u32 creator_id;
  u32 creator_revision;
} SDTHeader;

typedef struct PACK {
    SDTHeader header;
    u64       entry[];
} XSDT;
