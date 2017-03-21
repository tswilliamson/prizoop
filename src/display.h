#pragma once

void SetupDisplayDriver(bool withStretch, char withFrameskip);
void SetupDisplayColors(unsigned short bg0, unsigned short bg1, unsigned short bg2, unsigned short bg3, unsigned short sp0, unsigned short sp1, unsigned short sp2, unsigned short sp3);

void DmaWaitNext(void);

extern void(*renderScanline)(void);
extern void(*renderBlankScanline)(void);
extern void (*drawFramebuffer)(void);
