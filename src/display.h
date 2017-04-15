#pragma once

void SetupDisplayDriver(bool withStretch, char withFrameskip);
void SetupDisplayPalette(unsigned int pal[12]);

void DmaWaitNext(void);

extern void(*renderScanline)(void);
extern void(*renderBlankScanline)(void);
extern void (*drawFramebuffer)(void);
