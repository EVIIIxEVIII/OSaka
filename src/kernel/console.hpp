#pragma once

#include "shared.h"

static int cursorX = 0;
static int cursorY = 0;
static FramebufferInfo fb;

void console_set_fb(FramebufferInfo *p);
void clear_screen();
void putchar(char c);
void printk(const char* str, ...);
