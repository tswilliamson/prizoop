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

// resolve 2 blended scanlines to three 1.5x lines of 24-bit wide pixels (150%) with no blending (direct copy)
inline void BlendTripleScanline24(unsigned int* scanline1, unsigned int* scanline2, unsigned int* scanline3) {
	// 4 src pixels over 6 dest pixels (3 ints)
	int* src0 = &prevLineBuffer[7];
	int* src1 = &lineBuffer[7];
	for (int i = 0; i < 40; i++, scanline1 += 3, scanline2 += 3, scanline3 += 3, src0 += 4, src1 += 4) {
		const unsigned short color00 = (unsigned short) ppuPalette[src0[0]];
		const unsigned short color01 = (unsigned short) ppuPalette[src0[1]];
		const unsigned short color02 = (unsigned short) ppuPalette[src0[2]];
		const unsigned short color03 = (unsigned short) ppuPalette[src0[3]];
		const unsigned short color10 = (unsigned short) ppuPalette[src1[0]];
		const unsigned short color11 = (unsigned short) ppuPalette[src1[1]];
		const unsigned short color12 = (unsigned short) ppuPalette[src1[2]];
		const unsigned short color13 = (unsigned short) ppuPalette[src1[3]];

#ifdef LITTLE_E
		const unsigned int dest00 = (mix565(color00, color01) << 16) | color00;
		const unsigned int dest01 = (color02 << 16)  | color01;
		const unsigned int dest02 = (color03 << 16)  | mix565(color02, color03);
		const unsigned int dest20 = (mix565(color10, color11) << 16) | color10;
		const unsigned int dest21 = (color12 << 16)  | color11;
		const unsigned int dest22 = (color13 << 16)  | mix565(color12, color13);
#else
		const unsigned int dest00 = (color00 << 16)  | mix565(color00, color01);
		const unsigned int dest01 = (color01 << 16)  | color02;
		const unsigned int dest02 = (mix565(color02, color03) << 16) | color03;
		const unsigned int dest20 = (color10 << 16)  | mix565(color10, color11);
		const unsigned int dest21 = (color11 << 16)  | color12;
		const unsigned int dest22 = (mix565(color12, color13) << 16) | color13;
#endif

		scanline1[0] = dest00;
		scanline1[1] = dest01;
		scanline1[2] = dest02;
		scanline2[0] = mix565_32(dest00, dest20);
		scanline2[1] = mix565_32(dest01, dest21);
		scanline2[2] = mix565_32(dest02, dest22);
		scanline3[0] = dest20;
		scanline3[1] = dest21;
		scanline3[2] = dest22;
	}
}

// resolve 2 scanlines to three 1.5x lines of 24-bit wide pixels (150%) with no blending (direct copy)
inline void DirectTripleScanline24(unsigned int* scanline1, unsigned int* scanline2, unsigned int* scanline3) {
	// 4 src pixels over 6 dest pixels (3 ints)
	int* src0 = &prevLineBuffer[7];
	int* src1 = &lineBuffer[7];
	for (int i = 0; i < 40; i++, scanline1 += 3, scanline2 += 3, scanline3 += 3, src0 += 4, src1 += 4) {
		// first two pixels are just the src[0] twice
		scanline1[0] = ppuPalette[src0[0]];		
		const unsigned int color10 = ppuPalette[src1[0]];
		scanline2[0] = color10;
		scanline3[0] = color10;

#ifdef LITTLE_E
		const unsigned short color01 = (unsigned short)ppuPalette[src0[1]];
		const unsigned short color02 = (unsigned short)ppuPalette[src0[2]];
		const unsigned int   color03 = ppuPalette[src0[3]]; // no cast needed since bit shifted
		const unsigned short color11 = (unsigned short)ppuPalette[src1[1]];
		const unsigned short color12 = (unsigned short)ppuPalette[src1[2]];
		const unsigned int   color13 = ppuPalette[src1[3]]; // no cast needed since bit shifted
		
		scanline1[1] = (color02 << 16) | color01;
		scanline1[2] = (color03 << 16) | color02;

		const unsigned int dest1a = (color12 << 16) | color11;
		scanline2[1] = dest1a;
		scanline3[1] = dest1a;

		const unsigned int dest1b = (color13 << 16) | color12;
		scanline2[2] = dest1b;
		scanline3[2] = dest1b;
#else
		const unsigned int   color01 = ppuPalette[src0[1]]; // no cast needed since bit shifted
		const unsigned short color02 = (unsigned short)ppuPalette[src0[2]];
		const unsigned short color03 = (unsigned short)ppuPalette[src0[3]];
		const unsigned int   color11 = ppuPalette[src1[1]]; // no cast needed since bit shifted
		const unsigned short color12 = (unsigned short)ppuPalette[src1[2]];
		const unsigned short color13 = (unsigned short)ppuPalette[src1[3]];

		scanline1[1] = (color01 << 16) | color02;
		scanline1[2] = (color02 << 16) | color03;

		const unsigned int dest1a = (color11 << 16) | color12;
		scanline2[1] = dest1a;
		scanline3[1] = dest1a;

		const unsigned int dest1b = (color12 << 16) | color13;
		scanline2[2] = dest1b;
		scanline3[2] = dest1b;
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

// resolve a blended 32-bit scanline that is a mix of the line buffer with the previous line
inline void BlendMixedScanline32(unsigned int* dest) {
	// 4 src pixels over 6 dest pixels (3 ints)
	int* src = &lineBuffer[7];
	int* src2 = &prevLineBuffer[7];
	for (int i = 0; i < 160; i++, dest++, src++, src2++) {
		dest[0] = mix565_32(ppuPalette[*src], ppuPalette[*src2]);
	}
}

// resolve 2 blended scanlines to three 1.5x scaled 32-bit wide pixel lines simultenously
inline void BlendTripleScanline32(unsigned int* scanline1, unsigned int* scanline2, unsigned int* scanline3) {
	for (int i = 7; i < 167; i++) {
		const unsigned int color0 = ppuPalette[prevLineBuffer[i]];
		const unsigned int color1 = ppuPalette[lineBuffer[i]];
		*(scanline1++) = color0;
		*(scanline2++) = mix565_32(color0, color1);
		*(scanline3++) = color1;
	}
}
// resolve 2 scanlines to three 1.5x scaled 32-bit wide pixel lines simultenously
inline void DirectTripleScanline32(unsigned int* scanline1, unsigned int* scanline2, unsigned int* scanline3) {
	for (int i = 7; i < 167; i++) {
		const unsigned int color1 = ppuPalette[lineBuffer[i]];
		*(scanline1++) = ppuPalette[prevLineBuffer[i]];
		*(scanline2++) = color1;
		*(scanline3++) = color1;
	}
}