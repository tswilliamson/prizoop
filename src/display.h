#pragma once

void SetupDisplayDriver(bool withStretch, char withFrameskip);
void SetupDisplayColors(unsigned short c0, unsigned short c1, unsigned short c2, unsigned short c3);

void DmaWaitNext(void);

extern void (*renderScanline)(void);
extern void (*drawFramebuffer)(void);
