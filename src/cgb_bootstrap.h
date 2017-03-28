#pragma once

// function to support the color gameboy bootstrap that automagically selected color palettes for each game that came out before it
bool getCGBTableEntry(unsigned char* romInternalName, unsigned short* allPalettes);
