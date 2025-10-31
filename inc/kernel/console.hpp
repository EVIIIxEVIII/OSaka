#pragma once

#include "shared/shared.h"
#include "shared/types.h"

static int cursorX = 0;
static int cursorY = 0;
static FramebufferInfo fb;

void console_init(FramebufferInfo* p);
void clear_screen(u64 color);
void putchar(char c);
void printk(const char* str, ...);
