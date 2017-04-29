
#include <Windows.h>
#include <ShObjIdl.h>
#include <atlbase.h>
#include <atlconv.h>
#include <Shlobj.h>
#include <cstdio>

#include "../src/zx7/zx7.h"

bool FileOpen(char* intoPath)
{
	HRESULT hr;
	CComPtr<IFileOpenDialog> pDlg;
	COMDLG_FILTERSPEC aFileTypes[] = {
		{ L"Gameboy/ Gameboy Color ROM files", L"*.gb;*.gbc" },
	};

	// Create the file-open dialog COM object.
	hr = pDlg.CoCreateInstance ( __uuidof(FileOpenDialog) );

	if (FAILED(hr))
		return false;

	// Set the dialog's caption text and the available file types.
	// NOTE: Error handling omitted here for clarity.
	pDlg->SetFileTypes ( _countof(aFileTypes), aFileTypes );
	pDlg->SetTitle ( L"Select ROM file to compress" );

	// Show the dialog.
	hr = pDlg->Show ( NULL );
	// If the user chose a file, show a message box with the
	// full path to the file.
	if ( SUCCEEDED(hr) )
	{
		CComPtr<IShellItem> pItem;

		hr = pDlg->GetResult ( &pItem );

		if ( SUCCEEDED(hr) )
		{
			LPOLESTR pwsz = NULL;

			hr = pItem->GetDisplayName ( SIGDN_FILESYSPATH, &pwsz );

			if ( SUCCEEDED(hr) )
			{
				USES_CONVERSION;
				LPCSTR szText = OLE2CA(pwsz);
				strcpy_s(intoPath, 512, szText);
				CoTaskMemFree ( pwsz );
				return true;
			}
		}
	}

	return false;
}

unsigned int EndianSwapShort(unsigned int v) {
	return (v >> 8) | ((v & 0xFF) << 8);
}

int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
	) {

	CoInitialize(NULL);

	char path[512];
	if (FileOpen(path)) {
		OutputDebugString(path);
		OutputDebugString("\n");

		// compressed rom format is thusly: 
		//  0x180 byte header
		//  2 byte word of num 4K pages (N) with 2 byte instruction overlap
		//  N 2 byte words of compressed page sizes (4098 if left uncompressed)
		//  N zx7 compressed pages
		FILE* f;
		if (fopen_s(&f, path, "rb")) {
			OutputDebugString("Couldn't open for ROM read!\n");
			return -1;
		}
		fseek(f, 0, SEEK_END);

		// number of pages
		unsigned int size = ftell(f);
		int numPages = size / 4096;

		// read ROM into memory
		unsigned char* rom = (unsigned char*) malloc(size+2);
		fseek(f, 0, SEEK_SET);
		fread(rom, 1, size, f);
		fclose(f);

		// create compressed rom
		char* period = strrchr(path, '.');
		char newPath[512];
		strncpy_s(newPath, 512, path, period - path);
		strcat_s(newPath, 512, ".gbz");
		if (fopen_s(&f, newPath, "wb")) {
			OutputDebugString("Couldn't open for ROM write!\n");
			return -1;
		}

		// progress bar stuff
		IProgressDialog *pd = NULL;
		HRESULT hr = hr = CoCreateInstance(CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER, IID_IProgressDialog, (LPVOID *) (&pd));
		if (pd) {
			pd->StartProgressDialog(NULL, NULL, PROGDLG_NORMAL | PROGDLG_AUTOTIME | PROGDLG_NOMINIMIZE, NULL);
			pd->SetTitle(L"Compressing ROM File...");
			pd->SetProgress(0, numPages);
		}

		// write out header
		fwrite(rom, 1, 0x180, f);

		// num chunks (big endian format)
		unsigned short numChunks = EndianSwapShort(numPages);
		fwrite(&numChunks, 1, 2, f);

		// write 0'd out sizes for now
		unsigned int tablePos = ftell(f);
		unsigned short zero = 0;
		for (int i = 0; i < numPages; i++) {
			fwrite(&zero, 1, 2, f);
		}

		// compress each 4k section and write it
		for (int i = 0; i < numPages; i++) {
			// include two bytes of instruction overlap
			unsigned char page[4098];
			memcpy(page, rom + i * 4096, 4098);

			unsigned char* compPage = 0;
			int size = ZX7Compress(page, 4098, &compPage);

			// ignore compression if not at least better than 10% reduction
			if (size && size < 3700) {
				// write out compressed page
				fwrite(compPage, 1, size, f);
			} else {
				// decompressed is better for this page
				size = 4098;
				fwrite(page, 1, 4098, f);
			}

			if (compPage) {
				free(compPage);
			}

			// output result size to table
			unsigned int pos = ftell(f);
			fseek(f, tablePos, SEEK_SET);
			unsigned short chunkSize = EndianSwapShort(size);
			fwrite(&chunkSize, 1, 2, f);
			tablePos += 2;
			fseek(f, pos, SEEK_SET);

			if (pd) {
				pd->SetProgress(i, numPages);
			}
		}

		if (pd) {
			pd->StopProgressDialog();
			pd->Release();
			pd = NULL;
		}

		fclose(f);
	}

	return 0;
}