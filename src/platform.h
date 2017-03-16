#pragma once

#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "fxcg\display.h"
#include "fxcg\keyboard.h"
#include "fxcg\file.h"
#include "fxcg\registers.h"
#include "fxcg\rtc.h"
#include "fxcg\system.h"

#if TARGET_WINSIM
#define ALIGN(x) alignas(x)
#define LITTLE_E
#else
#define ALIGN(x) __attribute__((aligned(x)))
#define BIG_E
#endif

// compile time assert, will throw negative subscript error
#define CT_ASSERT(cond) static char check##__LINE__ [(cond) ? 1 : -1];

#include "ScopeTimer.h"

extern void ScreenPrint(char* buffer);
extern void reset_printf();
#define printf(...) { char buffer[256]; memset(buffer, 0, 256); sprintf(buffer, __VA_ARGS__); ScreenPrint(buffer); }
