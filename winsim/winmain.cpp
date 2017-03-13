#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <crtdbg.h>
#include <direct.h>
#include "freeglut\include\GL\freeglut.h"
#include <gl/gl.h>

#include "prizmsim.h"


HDC renderContext;
GLuint screenTexture;
extern void DisplayGLUTScreen();

typedef bool (APIENTRY *PFNWGLSWAPINTERVALEXTPROC) (int interval);

void glutErrorFunc(const char* format, va_list ap) {
	char toBuffer[512];
	vsnprintf(toBuffer, 512, format, ap);

	OutputDebugString("GLUT Error:");
	OutputDebugString(toBuffer);
}

void glutWarningFunc(const char* format, va_list ap) {
	char toBuffer[512];
	vsnprintf(toBuffer, 512, format, ap);

	OutputDebugString("GLUT Warning:");
	OutputDebugString(toBuffer);
}

void StartGlut() {
	int arg = 0;
	glutInit(&arg, NULL);

	glutInitErrorFunc(glutErrorFunc);
	glutInitErrorFunc(glutWarningFunc);

	glutInitWindowSize(384 * 2, 216 * 2);
	glutCreateWindow("Prizm Sim");
	glutDisplayFunc(DisplayGLUTScreen);
	glutInitDisplayMode(GLUT_RGBA);

	glGenTextures(1, &screenTexture);
	glBindTexture(GL_TEXTURE_2D, screenTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 384, 216, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);

	renderContext = wglGetCurrentDC();

}

int main(int argc, char **argv) {
	char *filename = NULL;
	
	printf("Starting Windows Simulator...\n");

	// CRT reporting can be a little bonkers and exit without any indication to us:
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);

	StartGlut();

	int ret = simmain();

	printf("Shutting down Windows Simulator...\n");
	return ret;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
		case WM_CREATE:
			return 0;
			
		case WM_CLOSE:
			PostQuitMessage(0);
			return 0;
			
		case WM_DESTROY:
			return 0;
			
		case WM_KEYDOWN:
			switch(wParam) {
				case VK_ESCAPE:
					printf("ESC pressed, quitting!\n");
					PostQuitMessage(0);
					break;
			}
			return 0;
			
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
}
