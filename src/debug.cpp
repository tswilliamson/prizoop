#ifndef PS4
	#include <stdio.h>
#endif

#include "platform.h"

#include "registers.h"
#include "memory.h"
#include "cpu.h"
#include "interrupts.h"

#include "debug.h"

#if DEBUG
unsigned char realtimeDebugEnable = 0;
#endif

static int printY = 0;
void reset_printf() {
	Bdisp_AllClr_VRAM();
	Bdisp_PutDisp_DD();

	printY = 0;
}

void ScreenPrint(char* buffer) {
	int x = 5;
	bool newline = buffer[strlen(buffer) - 1] == '\n';
	if (newline) buffer[strlen(buffer) - 1] = 0;
	PrintMini(&x, &printY, buffer, 0, 0xffffffff, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0);
	printY = (printY + 18) % 224;
}


#ifdef WIN
void realtimeDebug(void) {
	char debugMessage[5000];
	char *debugMessageP = debugMessage;
	
	unsigned char instruction = readByte(cpu.registers.pc);
	unsigned short operand = 0;
	
	if(instructions[instruction].operandLength == 1) operand = (unsigned short)readByte(cpu.registers.pc + 1);
	if(instructions[instruction].operandLength == 2) operand = readShort(cpu.registers.pc + 1);
	
	if(instructions[instruction].operandLength) debugMessageP += sprintf(debugMessageP, instructions[instruction].disassembly, operand);
	else debugMessageP += sprintf(debugMessageP, instructions[instruction].disassembly);
	
	debugMessageP += sprintf(debugMessageP, "\n\nAF: 0x%04x\n", cpu.registers.af);
	debugMessageP += sprintf(debugMessageP, "BC: 0x%04x\n", cpu.registers.bc);
	debugMessageP += sprintf(debugMessageP, "DE: 0x%04x\n", cpu.registers.de);
	debugMessageP += sprintf(debugMessageP, "HL: 0x%04x\n", cpu.registers.hl);
	debugMessageP += sprintf(debugMessageP, "SP: 0x%04x\n", cpu.registers.sp);
	debugMessageP += sprintf(debugMessageP, "PC: 0x%04x\n", cpu.registers.pc);
	
	debugMessageP += sprintf(debugMessageP, "\nIME: 0x%02x\n", interrupt.master);
	debugMessageP += sprintf(debugMessageP, "IE: 0x%02x\n", interrupt.enable);
	debugMessageP += sprintf(debugMessageP, "IF: 0x%02x\n", interrupt.flags);
	
	debugMessageP += sprintf(debugMessageP, "\nContinue debugging?\n");
	
	realtimeDebugEnable = MessageBox(NULL, debugMessage, "Prizoop Breakpoint", MB_YESNO) == IDYES ? 1 : 0;
}

#ifdef DEBUG_JUMP
void debugJump(void) {
	static unsigned short lastPC = 0;
	
	if(cpu.registers.pc != lastPC) {
		printf("Jumped to 0x%04x\n", cpu.registers.pc);
		lastPC = cpu.registers.pc;
	}
}
#endif

void printRegisters(void) {
	printf("A: 0x%02x\n", cpu.registers.a);
	printf("F: 0x%02x\n", cpu.registers.f);
	printf("B: 0x%02x\n", cpu.registers.b);
	printf("C: 0x%02x\n", cpu.registers.c);
	printf("D: 0x%02x\n", cpu.registers.d);
	printf("E: 0x%02x\n", cpu.registers.e);
	printf("H: 0x%02x\n", cpu.registers.h);
	printf("L: 0x%02x\n", cpu.registers.l);
	printf("SP: 0x%04x\n", cpu.registers.sp);
	printf("PC: 0x%04x\n", cpu.registers.pc);
	printf("IME: 0x%02x\n", interrupt.master);
	printf("IE: 0x%02x\n", interrupt.enable);
	printf("IF: 0x%02x\n", interrupt.flags);
}
#endif

