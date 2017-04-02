#pragma once

#include "platform.h"

#if __GNUC__
#pragma GCC diagnostic ignored "-fpermissive"
#endif

struct keys1type {
	#ifdef LITTLE_E
		unsigned char a : 1;
		unsigned char b : 1;
		unsigned char select : 1;
		unsigned char start : 1;
	#else
		unsigned char start : 1;
		unsigned char select : 1;
		unsigned char b : 1;
		unsigned char a : 1;
	#endif
};

struct keys2type {
	#ifdef LITTLE_E
		unsigned char right : 1;
		unsigned char left : 1;
		unsigned char up : 1;
		unsigned char down : 1;
	#else
		unsigned char down : 1;
		unsigned char up : 1;
		unsigned char left : 1;
		unsigned char right : 1;
	#endif
};

struct keys_type {
	union {
		struct {
			union {
				struct keys1type k1;
				unsigned char keys1 : 4;
			};
			
			union {
				struct keys2type k2;
				unsigned char keys2 : 4;
			};
		};
		
		unsigned char c;
	};
};

extern keys_type keys;

bool keyDown_fast(unsigned char keyCode);