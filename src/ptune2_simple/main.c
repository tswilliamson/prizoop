#include <display_syscalls.h>
#include <keyboard_syscalls.h>

#include "Ptune2_direct.h"
 
void main(void) {
	int key;

	SaveDataF1();	// backup CPG register
	LoadDataF3();	// set F3 clock
	     
	while (1) {
        Bdisp_AllClr_VRAM();
        GetKey(&key);
		switch (key) {
		}
	}

	LoadDataF1();	// restore CPG register
 
	return;
}