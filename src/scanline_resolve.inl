#include "gpu.h"

// Shared between platforms, renders a scanline based on the unsigned short color palette to different types
// (we speed up 2x scaling by directly rendering a scanline to unsigned long)

// resolve scanline to 16 bit wide pixels (100%)
inline void DirectScanline16(unsigned int* scanline) {
	// middle 160 bytes of linebuffer go into scanline
	for (int i = 7; i < 167; i += 2) {
#ifdef LITTLE_E
		*(scanline++) = (ppuPalette[lineBuffer[i+1]] << 16) | ((unsigned short) ppuPalette[lineBuffer[i]]);
#else
		*(scanline++) = (ppuPalette[lineBuffer[i]] << 16) | ((unsigned short)ppuPalette[lineBuffer[i + 1]]);
#endif
	}
}

// resolve scanline to 24-bit wide pixels (150%) with no blending (direct copy)
inline void DirectScanline24(unsigned int* dest) {
	// 4 src pixels over 6 dest pixels (3 ints)
	int* src = &lineBuffer[7];
	for (int i = 0; i < 40; i++, dest += 3, src += 4) {
		dest[0] = ppuPalette[src[0]];		// first two pixels are just the src[0] twice

#ifdef LITTLE_E
		const unsigned short color1 = (unsigned short)ppuPalette[src[1]];
		const unsigned short color2 = (unsigned short)ppuPalette[src[2]];
		const unsigned int color3 = ppuPalette[src[3]]; // no cast needed since bit shifted

		dest[1] = (color2 << 16) | (color1);
		dest[2] = (color3 << 16) | color2;
#else
		const unsigned int color1 = ppuPalette[src[1]]; // no cast needed since bit shifted
		const unsigned short color2 = (unsigned short)ppuPalette[src[2]];
		const unsigned short color3 = (unsigned short)ppuPalette[src[3]];

		dest[1] = (color1 << 16) | (color2);
		dest[2] = (color2 << 16) | color3;
#endif
	}
}

// resolve scanline to 32-bit wide pixels (200%) with no blending (direct copy)
inline void DirectScanline32(unsigned int* scanline) {
	// middle 160 bytes of linebuffer go into scanline
	for (int i = 7; i < 167; i++) {
		*(scanline++) = ppuPalette[lineBuffer[i]];
	}
}

// resolve scanline to two 32-bit wide pixel lines simultenously (for speedy scale)
inline void DirectDoubleScanline32(unsigned int* scanline1, unsigned int* scanline2) {
	for (int i = 7; i < 167; i++) {
		*(scanline1++) = ppuPalette[lineBuffer[i]];
		*(scanline2++) = ppuPalette[lineBuffer[i]];
	}
}

// resolve scanline to blended 24-bit wide pixel lines (150%)
inline void BlendScanline24(unsigned int* dest) {
	// 4 src pixels over 6 dest pixels (3 ints)
	int* src = &lineBuffer[7];
	for (int i = 0; i < 40; i++, dest += 3, src += 4) {
		unsigned short color0 = (unsigned short) ppuPalette[src[0]];
		unsigned short color1 = (unsigned short) ppuPalette[src[1]];
		unsigned short color2 = (unsigned short) ppuPalette[src[2]];
		unsigned short color3 = (unsigned short) ppuPalette[src[3]];

		unsigned int color01 = mix565(color0, color1);
		unsigned int color23 = mix565(color2, color3);

#ifdef LITTLE_E
		dest[0] = (color01 << 16) | (color0);
		dest[1] = (color2 << 16) | (color1);
		dest[2] = (color3 << 16) | color23;
#else
		dest[0] = (color0 << 16) | (color01);
		dest[1] = (color1 << 16) | (color2);
		dest[2] = (color23 << 16) | color3;
#endif
	}
}

// resolve a blended 24-bit scanline that is a mix of the line buffer with the previous line
inline void BlendMixedScanline24(unsigned int* dest) {
	// 4 src pixels over 6 dest pixels (3 ints)
	int* src = &lineBuffer[7];
	int* src2 = &prevLineBuffer[7];
	for (int i = 0; i < 40; i++, dest += 3, src += 4, src2 += 4) {
		unsigned short color0 = mix565((unsigned short) ppuPalette[src[0]], (unsigned short) ppuPalette[src2[0]]);
		unsigned short color1 = mix565((unsigned short) ppuPalette[src[1]], (unsigned short) ppuPalette[src2[1]]);
		unsigned short color2 = mix565((unsigned short) ppuPalette[src[2]], (unsigned short) ppuPalette[src2[2]]);
		unsigned short color3 = mix565((unsigned short) ppuPalette[src[3]], (unsigned short) ppuPalette[src2[3]]);

		unsigned color01 = mix565(color0, color1);
		unsigned color23 = mix565(color2, color3);

#ifdef LITTLE_E
		dest[0] = (color01 << 16) | (color0);
		dest[1] = (color2 << 16) | (color1);
		dest[2] = (color3 << 16) | color23;
#else
		dest[0] = (color0 << 16) | (color01);
		dest[1] = (color1 << 16) | (color2);
		dest[2] = (color23 << 16) | color3;
#endif
	}
}

// resolve a blended 32-bit scanline that is a mix of the line buffer with the previous line
inline void BlendMixedScanline32(unsigned int* dest) {
	// 4 src pixels over 6 dest pixels (3 ints)
	int* src = &lineBuffer[7];
	int* src2 = &prevLineBuffer[7];
	for (int i = 0; i < 160; i++, dest++, src++, src2++) {
		dest[0] = mix565_32(ppuPalette[*src], ppuPalette[*src2]);
	}
}