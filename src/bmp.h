#pragma once

// copies the given 16-bit bmp file to the screen
void PutBMP(const char* filepath, int atX = 0, int atY = 0, int srcY = 0, int destHeight = -1);