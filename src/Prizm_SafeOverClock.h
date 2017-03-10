#pragma once

/*
	Prizm SafeOverClock

	This was part of the fantastic PTune 2 utility source code by Sarento.

	Safe overclocking of the PLL requires correct changes to the bus read/write wait settings,
	and the peripheral clock. This code was written to simplify its incorporation into 
	application that want to provide faster speed as an option to users without 
	damaging calculators.

	Suggested usage is to call SetSafeClockSpeed(speed) when you desire a boost, and then
	call SetSafeClockSpeed(SCS_Normal) in your quit handler set with SetQuitHandler(...)

	Best practices include
		: Only use at the permission or request of the user
		: Avoid switching the speed very often (like every frame) or at weird times (like in an interrupt/timer handler)
		: Try to keep code that does a lot of OS calls (like file reading) in normal speed 

	This utility was written to be able to safely work in conjunction with PTune 2 but users 
	should still be warned to be very careful when adjusting speed settings.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	SCS_Half,			// 50% normal speed, for those inclined
	SCS_Normal,
	SCS_Fast,			// 150% normal speed, allows for probably quick enough refresh rates (87 MHz)
	SCS_Double			// double speed, should DEFINITELY warn users if using (118 MHz)
} SafeClockSpeed;

extern void SetSafeClockSpeed(SafeClockSpeed withSpeed);

#ifdef __cplusplus
};
#endif