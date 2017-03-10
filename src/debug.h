#pragma once

#if DEBUG

// Debug enables
//#define DEBUG_STACK // prints writes and reads to the stack
//#define DEBUG_JUMP // prints jumps
//#define DEBUG_SPEED // disables speed limiting

extern unsigned char realtimeDebugEnable;

#define S(x) #x
#define S_(x) S(x)
#define S__LINE__ S_(__LINE__)
#define STEP_LINE { int _x = 0; int _y = 0; PrintMini(&_x, &_y, "Step: " S__LINE__, 0, 0xffffffff, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0); GetKey(&_x); }
#define STEP_LINE_INFO(...) { int _x = 0; int _y = 0; PrintMini(&_x, &_y, "Step: " S__LINE__, 0, 0xffffffff, 0, 0, COLOR_BLACK, COLOR_WHITE, 1, 0); printf(__VA_ARGS__);  GetKey(&_x); }

#ifdef WIN
	void realtimeDebug(void);
#else
	#define realtimeDebug()
#endif

#ifdef WIN
	#ifndef DEBUG_JUMP
		#define debugJump()
	#else
		void debugJump(void);
	#endif
#else
	#define debugJump()
#endif

#ifdef WIN
	void printRegisters(void);
#else
	#define printRegisters()
#endif

#endif