#pragma once

#if DEBUG

#include "registers.h"

#if TARGET_WINSIM
#include <Windows.h>

// debug mem writes to this address (Slow. in WinSim will spit out the last 50 instructions, their address, the contents of the registers and the 8 bytes before and after the given address)
#define DEBUG_MEMACCESS 0

#if DEBUG_MEMACCESS
#define DEBUG_TRACKINSTRUCTIONS 32
#define DebugWrite(address) { if (address == DEBUG_MEMACCESS) { HitMemAccess(); } }
#endif

#if DEBUG_TRACKINSTRUCTIONS
struct InstructionsHistory {
	char instr[64];
	registers_type regs;
};
extern int instr_slot;
extern InstructionsHistory instr_hist[DEBUG_TRACKINSTRUCTIONS];
extern void HitMemAccess();
#define DebugInstruction(...) { sprintf(instr_hist[instr_slot % DEBUG_TRACKINSTRUCTIONS].instr, __VA_ARGS__); instr_hist[(instr_slot++) % DEBUG_TRACKINSTRUCTIONS].regs = registers; }
#endif

#define OutputLog(...) { char buffer[1024]; sprintf_s(buffer, 1024, __VA_ARGS__); OutputDebugString(buffer); }

#else

///////////////////////////////////////////////////////////////////////////////////////////////////
// For "cowboy debugging" Prizm crashes

#define S(x) #x
#define S_(x) S(x)
#define S__LINE__ S_(__LINE__)
#define STEP_LINE { int _x = 0; int _y = 0; PrintMini(&_x, &_y, "Step: " S__LINE__, 0, 0xffffffff, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0); GetKey(&_x); }
#define STEP_LINE_INFO(...) { int _x = 0; int _y = 0; PrintMini(&_x, &_y, "Step: " S__LINE__, 0, 0xffffffff, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0); printf(__VA_ARGS__);  GetKey(&_x); }

// Output log only available on WinSim for the moment (add write to file option?)
#define OutputLog(...)

#endif

#endif

#ifndef DebugInstruction
#define DebugInstruction(...) 
#endif

#ifndef DebugWrite
#define DebugWrite(...) 
#endif