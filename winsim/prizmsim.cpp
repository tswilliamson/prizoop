
/*******************************************************************
  Windows sim of various Prizm SDK functions to make life easier.
 *******************************************************************/

#include <stdio.h>
#include "freeglut\include\GL\freeglut.h"
#include "prizmsim.h"
#include <ShlObj.h>

extern HDC renderContext;
extern GLuint screenTexture;

#define TO_COLORREF_16(x) ( ((x & 0xF800) << 8) | ((x & 0x07E0) << 5) | ((x & 0x001F) << 3) )
#define TO_16_FROM_32(x) ((unsigned short) ( ((x & 0x00F80000) >> 8) | ((x & 0x0000FC00) >> 5) | ((x & 0x000000F8) >> 3) ))

///////////////////////////////////////////////////////////////////////////////////////////////////
// System

void(*_quitHandler)() = NULL;

void SetQuitHandler(void(*handler)()) {
	_quitHandler = handler;
}

int RTC_GetTicks() {
	return GetTickCount() * 16 / 125;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Display Generics

// the actual CPU side screen buffer
unsigned short ScreenBuffer[216][384] = { 0 };

void Bdisp_EnableColor(int n) {
	//  only true color currently supported
	Assert(n == 1);
}

void EnableStatusArea(int) {
	// ignored, no status area on win sim
}

void Bdisp_AllClr_VRAM() {
	memset(ScreenBuffer, 0xFF, sizeof(ScreenBuffer));
	glutPostRedisplay();
	glutMainLoopEvent();
}

void Bdisp_PutDisp_DD() {
	glutPostRedisplay();
	glutMainLoopEvent();
}

void *GetVRAMAddress(void) {
	return ScreenBuffer;
}

// Actual OpenGL draw of CPU texture
void DisplayGLUTScreen() {
	// artificial "vsync", hold tab to fast forward
	if ((GetAsyncKeyState(VK_TAB) & 0x8000) == 0)
		Sleep(16);

	GLenum i = glGetError();
	glBindTexture(GL_TEXTURE_2D, screenTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 384, 216, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, ScreenBuffer);

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

// returns width in pixels printed
static int PrintTextHelper(HFONT Font, int height, int x, int y, const char* string, int color, int back_color) {
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

	// blit each row (have to y flip)
	for (int i = 0; i < rect.bottom; i++, y++) {
		if (y >= 0 && y <= 215) {
			for (int j = 0; j < rect.right; j++) {
				if (x + j < 384 && x + j >= 0) {
					ScreenBuffer[y][x+j] = TO_16_FROM_32(rows[(384 * (215 - i) + j)]);
				}
			}
		}
	}

	DeleteObject(Bitmap);
	DeleteObject(Compat);

	return rect.right;
}

static HFONT MiniFont = CreateFont(18, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, FF_ROMAN, "Times New Roman");

void PrintMini(int *x, int *y, const char *MB_string, int mode_flags, unsigned int xlimit, int P6, int P7, int color, int back_color, int writeflag, int P11) {
	x += PrintTextHelper(MiniFont, 18, *x, *y, MB_string, color, back_color);

	glutPostRedisplay();
	glutMainLoopEvent();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Keyboard handling

// Mapped virtual keys:
#define NUM_KEY_MAPS 50
DWORD vKeyToPrizmKey[NUM_KEY_MAPS][2] = {
//  F1-F6 - F1-F6
	{ VK_F1,			KEY_CTRL_F1 },
	{ VK_F2,			KEY_CTRL_F2 },
	{ VK_F3,			KEY_CTRL_F3 },
	{ VK_F4,			KEY_CTRL_F4 },
	{ VK_F5,			KEY_CTRL_F5 },
	{ VK_F6,			KEY_CTRL_F6 },
//  Directional Arrows - Arrow Keys
	{ VK_LEFT,			KEY_CTRL_LEFT },
	{ VK_RIGHT,			KEY_CTRL_RIGHT },
	{ VK_UP,			KEY_CTRL_UP },
	{ VK_DOWN,			KEY_CTRL_DOWN },
//  SHIFT/OPTN/VARS/MENU - ASDF 
	{ 'A',				KEY_CTRL_SHIFT },
	{ 'S',				KEY_CTRL_OPTN },
	{ 'D',				KEY_CTRL_VARS },
	{ 'F',				KEY_CTRL_MENU },
//  ALPHA/X^2/^/EXIT - ZXCV
	{ 'Z',				KEY_CTRL_ALPHA },
	{ 'X',				KEY_CHAR_SQUARE },
	{ 'C',				KEY_CHAR_POW },
	{ 'V',				KEY_CTRL_EXIT },
//  Row of X.theta.tan - TYUIOP
	{ 'T',				KEY_CTRL_XTT },
	{ 'Y',				KEY_CHAR_LOG },
	{ 'U',				KEY_CHAR_LN },
	{ 'I',				KEY_CHAR_SIN },
	{ 'O',				KEY_CHAR_COS },
	{ 'P',				KEY_CHAR_TAN },
//  Row of Ab/c - GHJKL;
	{ 'G',				KEY_CTRL_MIXEDFRAC },
	{ 'H',				KEY_CTRL_FD },
	{ 'J',				KEY_CHAR_LPAR },
	{ 'K',				KEY_CHAR_RPAR },
	{ 'L',				KEY_CHAR_COMMA },
	{ ';',				KEY_CHAR_STORE },
//  Numpad maps to bottom numpad
	{ VK_NUMPAD0,		KEY_CHAR_0 },
	{ VK_NUMPAD1,		KEY_CHAR_1 },
	{ VK_NUMPAD2,		KEY_CHAR_2 },
	{ VK_NUMPAD3,		KEY_CHAR_3 },
	{ VK_NUMPAD4,		KEY_CHAR_4 },
	{ VK_NUMPAD5,		KEY_CHAR_5 },
	{ VK_NUMPAD6,		KEY_CHAR_6 },
	{ VK_NUMPAD7,		KEY_CHAR_7 },
	{ VK_NUMPAD8,		KEY_CHAR_8 },
	{ VK_NUMPAD9,		KEY_CHAR_9 },
	{ VK_OEM_MINUS,		KEY_CHAR_MINUS },
	{ VK_OEM_PLUS,		KEY_CHAR_PLUS },
	{ VK_MULTIPLY,		KEY_CHAR_MULT },
	{ VK_DIVIDE,		KEY_CHAR_DIV },
	{ VK_OEM_PERIOD,	KEY_CHAR_DP },
//  DEL - Delete
	{ VK_DELETE,		KEY_CTRL_DEL },
//  EXP - Home
	{ VK_HOME,			KEY_CHAR_EXP },
//  (-) - End
	{ VK_END,			KEY_CHAR_PMINUS },
//  EXE - Enter
	{ VK_RETURN,		KEY_CTRL_EXE },
//  AC/On - Escape (though this will exit the program on simulator anyway)
	{ VK_ESCAPE,		KEY_CTRL_AC },
};

int GetKey(int* key) {
	int result = 0;
	while (!result) {
		for (int i = 0; i < NUM_KEY_MAPS; i++) {
			if ((GetAsyncKeyState(vKeyToPrizmKey[i][0]) & 0x8000) != 0) {
				result = vKeyToPrizmKey[i][1];
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
			if ((GetAsyncKeyState(vKeyToPrizmKey[i][0]) & 0x8000) != 0) {
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

int GetKey_SimFast(int keycode) {
	for (int i = 0; i < NUM_KEY_MAPS; i++) {
		if (vKeyToPrizmKey[i][1] == keycode) {
			if ((GetAsyncKeyState(vKeyToPrizmKey[i][0]) & 0x8000) != 0) {
				return 1;
			} else {
				return 0;
			}
		}
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// File IO

int Bfile_CloseFile_OS(int handle) {
	CloseHandle((HANDLE)(size_t)handle);
	return 0;
}

int Bfile_GetFileSize_OS(int handle) {
	return GetFileSize((HANDLE)(size_t)handle, NULL);
}

int Bfile_OpenFile_OS(const unsigned short *filenameW, int mode, int null) {
	char filename[256];
	char docFilename[512];
	OFSTRUCT ofStruct;

	WideCharToMultiByte(CP_ACP, 0, (LPWSTR)filenameW, -1, (LPSTR)filename, 256, NULL, NULL);
	if (strncmp(filename, "\\\\fls0\\", 7) == 0) {
		memmove(&filename[0], &filename[7], 256 - 7);
	}

	// prizm rom files go in My Documents / Prizm / ROM
	PWSTR path;
	if (SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &path) == S_OK) {
		WideCharToMultiByte(CP_ACP, 0, (LPWSTR)path, -1, (LPSTR)docFilename, 512, NULL, NULL);
		strcat(docFilename, "\\Prizm\\ROM\\");
		strcat(docFilename, filename);

		// todo : support more than reading
		return (int)OpenFile((LPCSTR)docFilename, &ofStruct, OF_READ);
	}
	else {
		return 0;
	}
}

int Bfile_ReadFile_OS(int handle, void *buf, int size, int readpos) {
	DWORD read = 0;
	if (readpos != -1) {
		SetFilePointer((HANDLE)(size_t)handle, readpos, NULL, FILE_BEGIN);
	}
	ReadFile((HANDLE)(size_t)handle, buf, size, &read, NULL);
	return read;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Miscellaneous

void Bfile_NameToStr_ncpy(char*dest, const unsigned short*src, size_t n) {
	WideCharToMultiByte(CP_ACP, 0, (LPWSTR)src, -1, dest, 256, NULL, NULL);
}

void Bfile_StrToName_ncpy(unsigned short *dest, const char *source, size_t n) {
	MultiByteToWideChar(CP_ACP, 0, source, -1, (LPWSTR)dest, 256);
}