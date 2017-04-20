#pragma once

void SetupDisplayDriver(char withFrameskip);
void SetupDisplayPalette();

void DmaWaitNext(void);

extern void(*renderScanline)(void);
extern void(*renderBlankScanline)(void);
extern void (*drawFramebuffer)(void);
