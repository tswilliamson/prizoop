#pragma once

#include "platform.h"

struct registers_type {
	struct {
		union {
			struct {
				#ifdef LITTLE_E
					unsigned char f;
					unsigned char a;
					unsigned short _padaf;
				#else
					unsigned short _padaf;
					unsigned char a;
					unsigned char f;
				#endif
			};
			unsigned int af;
		};
	};
	
	struct {
		union {
			struct {
				#ifdef LITTLE_E
					unsigned char c;
					unsigned char b;
					unsigned short _padbc;
				#else
					unsigned short _padbc;
					unsigned char b;
					unsigned char c;
				#endif
			};
			unsigned int bc;
		};
	};
	
	struct {
		union {
			struct {
				#ifdef LITTLE_E
					unsigned char e;
					unsigned char d;
					unsigned short _padde;
				#else
					unsigned short _padde;
					unsigned char d;
					unsigned char e;
				#endif
			};
			unsigned int de;
		};
	};
	
	struct {
		union {
			struct {
				#ifdef LITTLE_E
					unsigned char l;
					unsigned char h;
					unsigned short _padhl;
				#else
					unsigned short _padhl;
					unsigned char h;
					unsigned char l;
				#endif
			};
			unsigned int hl;
		};
	};
	
	unsigned int sp;
	unsigned int pc;
};
