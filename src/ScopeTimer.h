#pragma once

#if !TARGET_WINSIM && DEBUG

#include "fxcg/tmu.h"

struct ScopeTimer {
	unsigned int cycleCount;
	unsigned int numCounts;

	const char* funcName;
	int line;

	ScopeTimer* nextTimer;

	ScopeTimer(const char* withFunctionName, int withLine);

	inline void AddTime(unsigned short cycles) {
		cycleCount += cycles;
		numCounts++;
	}

	static ScopeTimer* firstTimer;
	static char debugString[128];			// per application debug string (placed on last row), usually FPS or similar
	static void InitSystem();
	static void DisplayTimes();
	static void Shutdown();
};

struct TimedInstance {
	unsigned int start;
	ScopeTimer* myTimer;

	inline TimedInstance(ScopeTimer* withTimer) : start(REG_TMU_TCNT_2), myTimer(withTimer) {
	}

	inline ~TimedInstance() {
		int elapsed = (int)(start - REG_TMU_TCNT_2);
		if (elapsed >= 0) {
			myTimer->AddTime(elapsed);
		}
	}
};

#define TIME_SCOPE() static ScopeTimer __timer(__FUNCTION__, __LINE__); TimedInstance __timeMe(&__timer);
#define TIME_SCOPE_NAMED(Name) static ScopeTimer __timer(#Name, __LINE__); TimedInstance __timeMe(&__timer);
#else
struct ScopeTimer {
	static void InitSystem() {}
	static void DisplayTimes() {}
	static void Shutdown() {}
};
#endif

#ifndef TIME_SCOPE
#define TIME_SCOPE() 
#define TIME_SCOPE_NAMED(Name) 
#endif