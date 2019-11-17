#include "platform.h"
#include "debug.h"

#include "registers.h"
#include "memory.h"
#include "cpu.h"
#include "interrupts.h"

#include "calctype/fonts/arial_small/arial_small.h"

#if DEBUG_MEMWRITE
unsigned short debugWriteAddress = 0;
#endif

#if DEBUG_BREAKPOINT
unsigned short debugBreakpoint = 0;
#endif

static int printY = 0;
void reset_printf() {
	printY = 0;
}

void ScreenPrint(char* buffer) {
	int x = 5;
	bool newline = buffer[strlen(buffer) - 1] == '\n';
	if (newline) buffer[strlen(buffer) - 1] = 0;
	CalcType_Draw(&arial_small, buffer, x, printY + 2, COLOR_WHITE, 0, 0);
	printY = (printY + arial_small.height) % 224;
}

#if DEBUG
#define BORDER "<><><><><><><><><><><><><><><><><><><><>\n"

void failedAssert(const char* assertion) {
	OutputLog("Failed assert:\n");
	OutputLog(assertion);
	OutputLog("\n");

#if TARGET_WINSIM
	DebugBreak();
#endif
}

void LogRegisters(void) {
	OutputLog("Registers:\n");
	OutputLog(BORDER);
	OutputLog("AF: 0x%04x\n", cpu.registers.af);
	OutputLog("BC: 0x%04x\n", cpu.registers.bc);
	OutputLog("DE: 0x%04x\n", cpu.registers.de);
	OutputLog("HL: 0x%04x\n", cpu.registers.hl);
	OutputLog("SP: 0x%04x\n", cpu.registers.sp);
	OutputLog("PC: 0x%04x\n", cpu.registers.pc);
	OutputLog("IME: 0x%02x IE: 0x%02x IF: 0x%02x\n", cpu.IME, cpu.memory.IE_intenable, cpu.memory.IF_intflag);
	OutputLog("Stack: 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x\n",
		readShort(cpu.registers.sp),
		readShort(cpu.registers.sp+2),
		readShort(cpu.registers.sp+4),
		readShort(cpu.registers.sp+6),
		readShort(cpu.registers.sp+8),
		readShort(cpu.registers.sp+10))
}

#if DEBUG_TRACKINSTRUCTIONS
int instr_slot = 0;
InstructionsHistory instr_hist[DEBUG_TRACKINSTRUCTIONS] = { 0 };

void LogInstructionsHistory() {
	OutputLog("Last %d instructions:\n", DEBUG_TRACKINSTRUCTIONS);
	OutputLog(BORDER);
	int i = 1;
	int curInstruction = instr_slot % DEBUG_TRACKINSTRUCTIONS;
	OutputLog(" #   PC      INSTR                               AF   BC   DE   HL   SP\n");
	do {
		if (instr_hist[curInstruction].regs.pc) {
			OutputLog("(%02d) 0x%04x: %s", i++, instr_hist[curInstruction].regs.pc - 1, instr_hist[curInstruction].instr);
			for (int j = 36; j > strlen(instr_hist[curInstruction].instr); j--) {
				OutputLog(" ");
			}
			OutputLog("%04x %04x %04x %04x %04x\n", instr_hist[curInstruction].regs.af, instr_hist[curInstruction].regs.bc, instr_hist[curInstruction].regs.de, instr_hist[curInstruction].regs.hl, instr_hist[curInstruction].regs.sp);
		}
		curInstruction = (curInstruction + 1) % DEBUG_TRACKINSTRUCTIONS;
	} while (curInstruction != (instr_slot % DEBUG_TRACKINSTRUCTIONS));
}
#endif

#if DEBUG_MEMWRITE
void HitMemAccess() {
	if (!cpu.clocks) return;

	OutputLog("Hit Mem Access! 0x%04x (val: 0x%02x)\n", debugWriteAddress, readByte(debugWriteAddress));
	OutputLog(BORDER);

	// display memory around access
	OutputLog("Memory around access:\n");
	OutputLog(BORDER);
	for (int i = -8; i <= 8; i++) {
		OutputLog("%02x ", readByte(debugWriteAddress + i));
	}
	OutputLog("\n");
	for (int i = -8; i <= 8; i++) {
		if (i == 0) {
			OutputLog("** ");
		}
		else {
			OutputLog("   ");
		}
	}
	OutputLog("\n");

	// display register values:
	LogRegisters();

	// display recent instructions:
	LogInstructionsHistory();

	DebugBreak();
}
#endif

#if DEBUG_BREAKPOINT
void HitBreakpoint() {
	OutputLog("Hit Breakpoint! 0x%04x\n", debugBreakpoint);
	OutputLog(BORDER);

	// display register values:
	LogRegisters();

	// display recent instructions:
	LogInstructionsHistory();

	DebugBreak();
}
#endif

#endif

