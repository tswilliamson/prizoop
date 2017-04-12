
/*******************************************************************
  Windows sim of various Prizm SDK functions to make life easier.
 *******************************************************************/

#include <stdio.h>
#include "freeglut\include\GL\freeglut.h"
#include "prizmsim.h"
#include <ShlObj.h>

extern HDC renderContext;
extern GLuint screenTexture;
extern HWND GWnd;

///////////////////////////////////////////////////////////////////////////////////////////////////
// System

void(*_quitHandler)() = NULL;

void _CallQuit() {
	if (_quitHandler) {
		_quitHandler();
		_quitHandler = NULL;
	}
}

void SetQuitHandler(void(*handler)()) {
	_quitHandler = handler;
}

int RTC_GetTicks() {
	return GetTickCount() * 16 / 125;
}

void OS_InnerWait_ms(int ms) {
	Sleep(ms);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Display Generics

// the actual CPU side screen buffer
unsigned short VRAM[216][384] = { 0 };

unsigned short BackgroundVRAM[216][384] = { 0 };

void Bdisp_EnableColor(int n) {
	//  only true color currently supported
	Assert(n == 1);
}

void EnableStatusArea(int) {
	// ignored, no status area on win sim
}

void SaveVRAM_1() {
	memcpy(BackgroundVRAM, VRAM, sizeof(VRAM));
}

void LoadVRAM_1() {
	memcpy(VRAM, BackgroundVRAM, sizeof(VRAM));
}

void Bdisp_AllClr_VRAM() {
	memset(VRAM, 0xFF, sizeof(VRAM));
	glutPostRedisplay();
	glutMainLoopEvent();
}

void Bdisp_PutDisp_DD() {
	glutPostRedisplay();
	glutMainLoopEvent();
}

void DrawFrame(int color) {
	// does nothing
}

void *GetVRAMAddress(void) {
	return VRAM;
}

void Bdisp_AreaClr(struct display_fill* area, unsigned char target, unsigned short color) {
	unsigned targetColor = area->mode == 0 ? COLOR_WHITE : color;

	if (area->mode != 4) {
		for (int y = area->y1; y <= area->y2; y++) {
			for (int x = area->x1; x <= area->x2; x++) {
				VRAM[y][x] = targetColor;
			}
		}
	}
	else {
		for (int y = area->y1; y <= area->y2; y++) {
			for (int x = area->x1; x <= area->x2; x++) {
				VRAM[y][x] = ~(VRAM[y][x]);
			}
		}
	}
}

// Actual OpenGL draw of CPU texture
void DisplayGLUTScreen() {
	static long lastTicks = 0;
	// artificial "vsync", hold tab to fast forward
	if ((GetAsyncKeyState(VK_TAB) & 0x8000) == 0) {
		long curTicks;
		while ((curTicks = GetTickCount()) - 8 < lastTicks) {
			Sleep(0);;
		}
		lastTicks = curTicks;
	}

	GLenum i = glGetError();
	glBindTexture(GL_TEXTURE_2D, screenTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 384, 216, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, VRAM);

	glPushMatrix();
	glLoadIdentity();
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);

	glTexCoord2f(0, 1);
	glVertex2f(-1, -1); // Upper left
	glTexCoord2f(1, 1);
	glVertex2f(1, -1); // Upper right
	glTexCoord2f(1, 0);
	glVertex2f(1, 1); // Lower right
	glTexCoord2f(0, 0);
	glVertex2f(-1, 1); // Lower left
	glEnd();
	glPopMatrix();

	glutSwapBuffers();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Text Printing

#define TO_COLORREF_16(x) ( ((x & 0xF800) << 8) | ((x & 0x07E0) << 5) | ((x & 0x001F) << 3) )
#define TO_16_FROM_32(x) ((unsigned short) ( ((x & 0x00F80000) >> 19) | ((x & 0x0000FC00) >> 5) | ((x & 0x000000F8) << 8) ))

// returns width in pixels printed
static int PrintTextHelper(HFONT Font, int height, int x, int y, const char* string, int color, int back_color, bool transparent, int writeflag = 1) {
	HDC Compat = CreateCompatibleDC(wglGetCurrentDC());
	HBITMAP Bitmap = CreateCompatibleBitmap(renderContext, 384, 216);

	::SelectObject(Compat, Bitmap);
	::SelectObject(Compat, Font);

	SetTextColor(Compat, TO_COLORREF_16(color));
	SetBkColor(Compat, TO_COLORREF_16(back_color));

	RECT rect;
	rect.top = 0;
	rect.bottom = height;
	rect.left = 0;
	rect.right = 600;
	int h = DrawText(Compat, (LPCSTR)string, -1, &rect, DT_CALCRECT);
	h = DrawText(Compat, (LPCSTR)string, -1, &rect, DT_SINGLELINE);

	BITMAPINFO info[2] = { 0 };
	info[0].bmiHeader.biSize = sizeof(info[0].bmiHeader);
	GetDIBits(Compat, Bitmap, 0, 216, 0, &info[0], DIB_RGB_COLORS);
	unsigned int* rows = (unsigned int*)malloc(info[0].bmiHeader.biSizeImage);
	Assert(info[0].bmiHeader.biBitCount == 32);
	GetDIBits(Compat, Bitmap, 0, 216, (LPVOID)rows, &info[0], DIB_RGB_COLORS);

	if (writeflag) {
		// blit each row (have to y flip)
		for (int i = 0; i < rect.bottom; i++, y++) {
			if (y >= 0 && y <= 215) {
				for (int j = 0; j < rect.right; j++) {
					if (x + j < 384 && x + j >= 0) {
						if (!transparent || TO_16_FROM_32(rows[(384 * (215 - i) + j)]) != back_color) {
							VRAM[y][x + j] = TO_16_FROM_32(rows[(384 * (215 - i) + j)]);
						}
					}
				}
			}
		}
	}

	free(rows);

	DeleteObject(Bitmap);
	DeleteObject(Compat);

	return rect.right;
}

static HFONT MiniFont = CreateFont(18, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, FF_ROMAN, "Times New Roman");

void PrintMini(int *x, int *y, const char *MB_string, int mode_flags, unsigned int xlimit, int P6, int P7, int color, int back_color, int writeflag, int P11) {
	*x += PrintTextHelper(MiniFont, 18, *x, *y, MB_string, color, back_color, (mode_flags & 0x40), writeflag);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Keyboard handling

struct VKeyMapping {
	DWORD virtualKey;
	unsigned short getKeyCode;
	unsigned char fastKeyCode;
};

// Mapped virtual keys:
#define NUM_KEY_MAPS 50
static VKeyMapping keys[NUM_KEY_MAPS] = {
//  F1-F6 - F1-F6
	{ VK_F1,			KEY_CTRL_F1, 79 },
	{ VK_F2,			KEY_CTRL_F2, 69 },
	{ VK_F3,			KEY_CTRL_F3, 59 },
	{ VK_F4,			KEY_CTRL_F4, 49 },
	{ VK_F5,			KEY_CTRL_F5, 39 },
	{ VK_F6,			KEY_CTRL_F6, 29 },
//  Directional Arrows - Arrow Keys
	{ VK_LEFT,			KEY_CTRL_LEFT, 38 },
	{ VK_RIGHT,			KEY_CTRL_RIGHT, 27 },
	{ VK_UP,			KEY_CTRL_UP, 28 },
	{ VK_DOWN,			KEY_CTRL_DOWN, 37 },
//  SHIFT/OPTN/VARS/MENU - ASDF 
	{ 'A',				KEY_CTRL_SHIFT, 78 },
	{ 'S',				KEY_CTRL_OPTN, 68 },
	{ 'D',				KEY_CTRL_VARS, 58 },
	{ 'F',				KEY_CTRL_MENU, 48 },
//  ALPHA/X^2/^/EXIT - ZXCV
	{ 'Z',				KEY_CTRL_ALPHA, 77 },
	{ 'X',				KEY_CHAR_SQUARE, 67 },
	{ 'C',				KEY_CHAR_POW, 57 },
	{ 'V',				KEY_CTRL_EXIT, 47 },
//  Row of X.theta.tan - TYUIOP
	{ 'T',				KEY_CTRL_XTT, 76 },
	{ 'Y',				KEY_CHAR_LOG, 66 },
	{ 'U',				KEY_CHAR_LN, 56 },
	{ 'I',				KEY_CHAR_SIN, 46 },
	{ 'O',				KEY_CHAR_COS, 36 },
	{ 'P',				KEY_CHAR_TAN, 26 },
//  Row of Ab/c - GHJKL;
	{ 'G',				KEY_CTRL_MIXEDFRAC, 75 },
	{ 'H',				KEY_CTRL_FD, 65 },
	{ 'J',				KEY_CHAR_LPAR, 55 },
	{ 'K',				KEY_CHAR_RPAR, 45 },
	{ 'L',				KEY_CHAR_COMMA, 35 },
	{ VK_OEM_1,			KEY_CHAR_STORE, 25 },		// semicolon
//  Numpad maps to bottom numpad
	{ VK_NUMPAD0,		KEY_CHAR_0, 71 },
	{ VK_NUMPAD1,		KEY_CHAR_1, 72 },
	{ VK_NUMPAD2,		KEY_CHAR_2, 62 },
	{ VK_NUMPAD3,		KEY_CHAR_3, 52 },
	{ VK_NUMPAD4,		KEY_CHAR_4, 73 },
	{ VK_NUMPAD5,		KEY_CHAR_5, 63 },
	{ VK_NUMPAD6,		KEY_CHAR_6, 53 },
	{ VK_NUMPAD7,		KEY_CHAR_7, 74 },
	{ VK_NUMPAD8,		KEY_CHAR_8, 64 },
	{ VK_NUMPAD9,		KEY_CHAR_9, 54 },
	{ VK_OEM_MINUS,		KEY_CHAR_MINUS, 32 },
	{ VK_OEM_PLUS,		KEY_CHAR_PLUS, 42 },
	{ VK_MULTIPLY,		KEY_CHAR_MULT, 43 },
	{ VK_DIVIDE,		KEY_CHAR_DIV, 33 },
	{ VK_OEM_PERIOD,	KEY_CHAR_DP, 61 },
//  DEL - Delete
	{ VK_DELETE,		KEY_CTRL_DEL, 44 },
//  EXP - Home
	{ VK_HOME,			KEY_CHAR_EXP , 51},
//  (-) - End
	{ VK_END,			KEY_CHAR_PMINUS, 41 },
//  EXE - Enter
	{ VK_RETURN,		KEY_CTRL_EXE, 31 },
//  AC/On - Escape (though this will exit the program on simulator anyway)
	{ VK_ESCAPE,		KEY_CTRL_AC, 10 },
};

int GetKey(int* key) {
	int result = 0;
	while (!result) {
		for (int i = 0; i < NUM_KEY_MAPS; i++) {
			if ((GetFocus() == GWnd && GetAsyncKeyState(keys[i].virtualKey) & 0x8000) != 0) {
				result = keys[i].getKeyCode;
				break;
			}
		}
		if (!result) {
			glutPostRedisplay();
			glutMainLoopEvent();
		}
	}

	// wait for no keys to be up
	bool up = false;
	while (!up) {
		up = true;
		for (int i = 0; i < NUM_KEY_MAPS; i++) {
			if ((GetAsyncKeyState(keys[i].virtualKey) & 0x8000) != 0) {
				up = false;
				break;
			}
			if (!up) {
				glutPostRedisplay();
				glutMainLoopEvent();
			}
		}
	};

	if (result == KEY_CTRL_AC) {
		PostQuitMessage(0);
	}

	*key = result;
	return result >= 30000 ? 0 : 1;
}

bool keyDown_fast(unsigned char keycode) {
	for (int i = 0; i < NUM_KEY_MAPS; i++) {
		if (keys[i].fastKeyCode == keycode) {
			if ((GetFocus() == GWnd && GetAsyncKeyState(keys[i].virtualKey) & 0x8000) != 0) {
				return true;
			} else {
				return false;
			}
		}
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// File IO

static char romPath[256] = { 0 };
static char ramPath[256] = { 0 };

static void ResolvePaths() {
	if (romPath[0] == 0) {
		PWSTR path;
		if (SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &path) == S_OK) {
			WideCharToMultiByte(CP_ACP, 0, (LPWSTR)path, -1, (LPSTR)romPath, 256, NULL, NULL);

			strcpy(ramPath, romPath);

			// prizm rom files go in My Documents / Prizm / ROM
			strcat(romPath, "\\Prizm\\ROM\\");

			// prizm ram files go in My Documents / Prizm / RAM
			strcat(ramPath, "\\Prizm\\RAM\\");
		}
	}
}

static bool ResolveROMPath(const unsigned short* prizmPath, char* intoPath) {
	char filename[256];
	WideCharToMultiByte(CP_ACP, 0, (LPWSTR)prizmPath, -1, (LPSTR)filename, 256, NULL, NULL);

	// must begin with filesystem path
	if (strncmp(filename, "\\\\fls0\\", 7)) {
		return false;
	}
	memmove(&filename[0], &filename[7], 256 - 7);

	// construct path to ROM in Windows docs
	ResolvePaths();
	strcpy(intoPath, romPath);
	strcat(intoPath, filename);

	return true;
}

int Bfile_OpenFile_OS(const unsigned short *filenameW, int mode, int null) {
	char docFilename[512];
	if (!ResolveROMPath(filenameW, docFilename)) {
		return -5;
	}

	HFILE result;
	OFSTRUCT ofStruct;
	switch (mode) {
		case READ:
			result = OpenFile((LPCSTR)docFilename, &ofStruct, OF_READ);
			break;
		case WRITE:
			// no create here, have to call Bfile_CreateEntry_OS to create first
			result = OpenFile((LPCSTR)docFilename, &ofStruct, OF_WRITE);
			break;
		case READWRITE:
			result = OpenFile((LPCSTR)docFilename, &ofStruct, OF_READ | OF_WRITE);
			break;
		default:
			// remaining cases result in error
			result = HFILE_ERROR;
			break;
	}
	if (result == HFILE_ERROR)
		return -1;
	return (int) result;
}

int Bfile_CreateEntry_OS(const unsigned short*filename, int mode, size_t *size) {
	char docFilename[512];
	if (!ResolveROMPath(filename, docFilename)) {
		return -5;
	}

	if (mode == CREATEMODE_FOLDER) {
		if (CreateDirectory(docFilename, NULL)) {
			return 0;
		}
		return -1;
	}
	else if (mode == CREATEMODE_FILE) {
		OFSTRUCT ofStruct;
		HFILE file = OpenFile(docFilename, &ofStruct, OF_CREATE | OF_WRITE);
		if (file == HFILE_ERROR)
			return -1;

		int zero = 0;
		DWORD written;
		for (int i = 0; i < *size; i += 4) {
			WriteFile((HANDLE) file, &zero, 4, &written, NULL);
		}
		CloseHandle((HANDLE) file);
		return 0;
	}

	// unknown mode
	return -1;
}

int Bfile_SeekFile_OS(int handle, int pos) {
	SetFilePointer((HANDLE)(size_t)handle, pos, NULL, FILE_BEGIN);
	return pos;
}

int Bfile_ReadFile_OS(int handle, void *buf, int size, int readpos) {
	DWORD read = 0;
	if (readpos != -1) {
		SetFilePointer((HANDLE)(size_t)handle, readpos, NULL, FILE_BEGIN);
	}
	ReadFile((HANDLE)(size_t)handle, buf, size, &read, NULL);
	return read;
}

int Bfile_WriteFile_OS(int handle, const void* buf, int size) {
	DWORD written;
	WriteFile((HANDLE)handle, buf, size, &written, NULL);
	if (written == size) {
		return SetFilePointer((HANDLE)handle, 0, NULL, FILE_CURRENT);
	}
	return -1;
}

int Bfile_GetFileSize_OS(int handle) {
	return GetFileSize((HANDLE)(size_t)handle, NULL);
}

int Bfile_CloseFile_OS(int handle) {
	CloseHandle((HANDLE)(size_t)handle);
	return 0;
}

struct simFindData {
	unsigned short id, type;
	unsigned long fsize, dsize;
	unsigned int property;
	unsigned long address;
	HANDLE winHandle;
	WIN32_FIND_DATA winData;

	void Fill() {
		fsize = winData.nFileSizeLow;
		dsize = winData.nFileSizeLow;
	}
};

int Bfile_FindFirst(const char *pathAsWide, int *FindHandle, char *foundfile, void *fileinfo) {
	char docPath[512];
	OFSTRUCT ofStruct;

	*FindHandle = 0;

	if (!ResolveROMPath((unsigned short*) pathAsWide, docPath)) {
		return -5;
	}

	simFindData* data = new simFindData;
	data->winHandle = FindFirstFile(docPath, &data->winData);

	if (data->winHandle == INVALID_HANDLE_VALUE) {
		delete data;
		return -1;
	}
	else {
		data->Fill();
		MultiByteToWideChar(CP_ACP, 0, (LPSTR)data->winData.cFileName, -1, (LPWSTR) foundfile, 256);
		if (fileinfo) memcpy(fileinfo, data, 20);
		*FindHandle = (int) data;
		return 0;
	}
}

int Bfile_FindNext(int FindHandle, char *foundfile, char *fileinfo) {
	simFindData* data = (simFindData*)FindHandle;
	if (data) {
		BOOL ret = FindNextFile(data->winHandle, &data->winData);
		if (ret) {
			data->Fill();
			MultiByteToWideChar(CP_ACP, 0, (LPSTR)data->winData.cFileName, -1, (LPWSTR)foundfile, 256);
			if (fileinfo) memcpy(fileinfo, data, 20);
			return 0;
		} else {
			return -16;
		}
	}
	return -1;
}

int Bfile_FindClose(int FindHandle) {
	simFindData* data = (simFindData*)FindHandle;
	if (data) {
		FindClose(data->winHandle);
		delete data;
	}
	return 0;
}

int MCS_CreateDirectory(unsigned char*dir) {
	if (!dir || dir[0] == 0)
		return 0xF0;

	ResolvePaths();

	char fullPath[512];
	strcpy(fullPath, ramPath);
	strcat(fullPath, (const char*) dir);
	if (CreateDirectory(fullPath, NULL)) {
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			return 0x42;
		} else {
			return 0x43;
		}
	}

	return 0;
}

static char curReadFile[512] = { 0 };

int MCSGetData1(int offset, int len_to_copy, void*buffer) {
	if (curReadFile[0] == 0) {
		return -1;
	}

	OFSTRUCT ofStruct;
	HFILE file = OpenFile(curReadFile, &ofStruct, OF_READ);
	if (file == HFILE_ERROR) {
		curReadFile[0] = 0;
		return -1;
	}

	SetFilePointer((HANDLE)file, offset, NULL, FILE_BEGIN);

	DWORD read = 0;
	ReadFile((HANDLE)file, buffer, len_to_copy, &read, NULL);
	CloseHandle((HANDLE)file);

	if (read != len_to_copy)
		return -1;

	return 0;
}

int MCSGetDlen2(unsigned char*dir, unsigned char*item, int*data_len) {
	ResolvePaths();
	strcpy(curReadFile, ramPath);
	strcat(curReadFile, (const char*)dir);
	strcat(curReadFile, "/");
	strcat(curReadFile, (const char*)item);

	OFSTRUCT ofStruct;
	HFILE file = OpenFile(curReadFile, &ofStruct, OF_READ);
	if (file == HFILE_ERROR) {
		curReadFile[0] = 0;
		return -1;
	}

	*data_len = GetFileSize((HANDLE) file, NULL);
	CloseHandle((HANDLE)file);

	return 0;
}

int MCS_WriteItem(unsigned char*dir, unsigned char*item, short itemtype, int data_length, int buffer) {
	ResolvePaths();

	if (data_length % 4 != 0)
		return 0x10;

	char fullPath[512];
	strcpy(fullPath, ramPath);
	strcat(fullPath, (const char*)dir);
	strcat(fullPath, "/");
	strcat(fullPath, (const char*) item);

	OFSTRUCT ofStruct;
	HFILE file = OpenFile(fullPath, &ofStruct, OF_CREATE | OF_WRITE);
	if (file == HFILE_ERROR)
		return 0x40;

	DWORD written = 0;
	WriteFile((HANDLE)file, (void*) buffer, data_length, &written, NULL);
	CloseHandle((HANDLE)file);

	if (written != data_length)
		return 0x11;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Miscellaneous

void Bfile_NameToStr_ncpy(char*dest, const unsigned short*src, size_t n) {
	WideCharToMultiByte(CP_ACP, 0, (LPWSTR)src, -1, dest, 256, NULL, NULL);
}

void Bfile_StrToName_ncpy(unsigned short *dest, const char *source, size_t n) {
	MultiByteToWideChar(CP_ACP, 0, source, -1, (LPWSTR)dest, 256);
}