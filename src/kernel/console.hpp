#pragma once

#include "shared.h"
#include "types.h"

static int cursorX = 0;
static int cursorY = 0;
static FramebufferInfo fb;

void console_set_fb(FramebufferInfo *p);
void clear_screen(u64 color);
void putchar(char c);
void printk(const char* str, ...);
