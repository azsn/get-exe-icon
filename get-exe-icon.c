// Copyright (c) 2021 Aury Snow
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#define WIN32_LEAN_AND_MEAN
#include "get-exe-icon.h"
#include <stdlib.h>
#include <stdint.h>

// Notes about the code:
//
// This code extracts the primary icon resource used by an EXE or DLL to
// an .ICO file format exactly as the original application developer
// compiled it into the executable.
//
// An .ICO file is made up of a header, a simple directory listing of
// all images within the file, and then all of the images in order. If
// an image is a BMP, its header is not present. Otherwise, the entire
// image is present (for example, a PNG inside an ICO could be extracted
// verbatim to a .PNG file).
//
// When an .ICO is compiled into an EXE/DLL, it is split into multiple
// resources: a single RT_GROUP_ICON resource which contains the ICO
// header and (slightly modified) directories, and then a RT_ICON for
// each image in the ICO file. Instead of the directories pointing to
// the byte offset of their associated image as they do in .ICO files,
// the directories in the RT_GROUP_ICON resource state the resource
// number of their associated image. Everything else is exactly the same
// as in the file.
//
// Therefore, extracting the original .ICO amounts to loading the first
// enumerated RT_GROUP_ICON resource, rebuilding the .ICO file header
// and directory structure from it, and then copying the contents of
// each RT_ICON resource it references verbatim into the output .ICO
// file. Some links to information on the topic:
//
// https://stackoverflow.com/questions/3270757/in-resources-of-a-executable-file-how-does-one-find-the-default-icon
// https://stackoverflow.com/questions/20729156/find-out-number-of-icons-in-an-icon-resource-using-win32-api
// https://devblogs.microsoft.com/oldnewthing/?p=7083
// https://stackoverflow.com/questions/3350852/how-to-correctly-fix-zero-sized-array-in-struct-union-warning-c4200-without
// https://docs.microsoft.com/en-us/windows/desktop/debug/retrieving-the-last-error-code
//
// There are other ways to achieve icon extraction which may be more
// useful in other situations. If you just want the system's "large"
// or "small" icons, ExtractIconA() or ExtractAssociatedIconA() could
// be used. However, these will only extract the 32x32 or 16x16 icon
// (as an HICON). To extact an HICON of any size (even sizes that do
// not exist in the original .ICO file -- it scales automatically), use
// SHDefExtractIconW(). Example extraction of 64x64 icon from an EXE:
//
// HICON hIcon;
// if (SHDefExtractIconW(exeFilePath, 0, 0, &hIcon, NULL, MAKELPARAM(64, 0)) != S_OK) {
// 	// Error
// }
//
// The problem with these methods is that they all give you an HICON,
// which is difficult to reconstruct into an ICO file: the information
// about what sizes are present in the original ICO is lost, all sizes
// are converted into bitmaps (.ICOs can contain PNGs too), and most
// importantly, correctly generating a BMP from an HICON is much more
// difficult than it seems like it should be. Some links to information
// on the topic:
//
// https://stackoverflow.com/questions/37370241/difference-between-extracticon-and-extractassociatedicon-need-to-extract-icon-o
// https://stackoverflow.com/questions/2289894/how-can-i-save-hicon-to-an-ico-file
// https://stackoverflow.com/questions/7375003/how-to-convert-hicon-to-hbitmap-in-vc
// http://codewee.com/view.php?idx=53 (Save HBITMAP to file)
// https://social.msdn.microsoft.com/Forums/vstudio/en-US/d08a599a-e1cc-4219-b76b-5bd0b4ea58d1/win32-how-use-getdibits-and-setdibits?forum=vclanguage


// ICO header format, which is the same on disk and in a resource.
// This header is immediately followed by idCount DiskIcoDirEntrys
// or ResIcoDirEntrys, depending on whether it's on disk or in a
// resource, respectively. See ICO format specification for details.
#pragma pack( push )
#pragma pack( 2 )
typedef struct
{
	uint16_t  reserved;
	uint16_t  type;
	uint16_t  count;
} IcoHeader;
#pragma pack( pop )

// ICO Directory Entry format ON DISK.
#pragma pack( push )
#pragma pack( 2 )
typedef struct
{
	uint8_t   width;
	uint8_t   height;
	uint8_t   colorCount;
	uint8_t   reserved;
	uint16_t  planes;
	uint16_t  bitCount;
	uint32_t  sizeBytes;
	uint32_t  offset;       // Global offset of image in file
} DiskIcoDirEntry;
#pragma pack( pop )

// ICO Directory Entry format IN A RESOURCE.
#pragma pack( push )
#pragma pack( 2 )
typedef struct
{
	uint8_t   width;
	uint8_t   height;
	uint8_t   colorCount;
	uint8_t   reserved;
	uint16_t  planes;
	uint16_t  bitCount;
	uint32_t  sizeBytes;
	uint16_t  resId;        // RT_ICON resource ID
} ResIcoDirEntry;
#pragma pack( pop )

// Finds, loads, and "locks" (gets a pointer to) a resource
// The returned resource pointer does not need to be freed,
// it will be released when the module is unloaded/freed.
static LPCVOID get_resource(HMODULE module, LPCSTR name, LPCSTR type, DWORD *len)
{
	HRSRC resInfo = FindResourceA(module, name, type);
	if (!resInfo) {
		return NULL;
	}

	DWORD dataLen = SizeofResource(module, resInfo);
	if (dataLen == 0) {
		return NULL;
	}

	if (len) {
		*len = dataLen;
	}

	HGLOBAL res = LoadResource(module, resInfo);
	if (!res) {
		return NULL;
	}

	return LockResource(res);
}

static BOOL is_png(const BYTE *data, DWORD len) {
	return len >= 8
	    && data[0] == 137
	    && data[1] == 80
	    && data[2] == 78
	    && data[3] == 71
	    && data[4] == 13
	    && data[5] == 10
	    && data[6] == 26
	    && data[7] == 10;
}

// Takes a module and a RT_GROUP_ICON resource identifier name that it contains
// and converts it into an .ICO file, stored in a byte buffer. This buffer could
// be written directly to disk and opened as an .ICO. If an error occurs, the NULL
// is returned and an error message is printed to stderr.
// Free the returned buffer with free().
// ICOs may use PNGs instead of bitmaps for individual image entries, however
// not all programs support this. Use 'allowEmbeddedPNGs' to enable or disable
// including PNGs in the ICO output.
static PBYTE extract_ico_from_module(HMODULE module, PCSTR name, BOOL allowEmbeddedPNGs, PDWORD bufLen)
{
	const IcoHeader *header = (const IcoHeader *)get_resource(module,
		name,
		(LPSTR)RT_GROUP_ICON,
		NULL);
	if (!header) {
		return NULL;
	}

	// ICO directory entries in the module's resources (directly follows
	// the RT_GROUP_ICON resource header)
	const ResIcoDirEntry *resDirEntries
		= (ResIcoDirEntry *)((const BYTE *)header + sizeof(IcoHeader));

	const BYTE **imgDatas = (const BYTE **)calloc(sizeof(BYTE *), header->count);
	DWORD *imgDataLens = (DWORD *)calloc(sizeof(DWORD), header->count);

	*bufLen = sizeof(IcoHeader);
	uint16_t imgs = 0; // Num images in output ICO <= header->count

	// Determine the output ICO (icoBuf) size, while throwing
	// out undesired images (optionally PNGs)
	for (uint16_t i = 0; i < header->count; i++) {
		DWORD imgDataLen;
		const BYTE *imgData = (const BYTE *)get_resource(module,
			MAKEINTRESOURCEA(resDirEntries[i].resId),
			(PSTR)RT_ICON,
			&imgDataLen);
		if (!imgData) {
			continue;
		}

		if (!allowEmbeddedPNGs && is_png(imgData, imgDataLen)) {
			continue;
		}

		*bufLen += sizeof(DiskIcoDirEntry);
		*bufLen += imgDataLen;

		imgDatas[i] = imgData;
		imgDataLens[i] = imgDataLen;
		imgs ++;
	}

	if (imgs == 0) {
		free((void *)imgDatas);
		free(imgDataLens);
		return NULL;
	}

	PBYTE icoBuf = (PBYTE)malloc(*bufLen);
	if (!icoBuf) {
		free((void *)imgDatas);
		free(imgDataLens);
		return NULL;
	}

	IcoHeader mHeader;
	memcpy(&mHeader, header, sizeof(IcoHeader));
	mHeader.count = imgs;

	// The beginning of a RT_GROUP_ICON resource is exactly equivalent to
	// the contents of an ICO header on disk. Write it out verbatim (with
	// the updated 'count' field).
	memcpy(&icoBuf[0], &mHeader, sizeof(IcoHeader));

	// ICO directory entries 'on disk' (in the icoBuf)
	DiskIcoDirEntry *diskDirEntries
		= (DiskIcoDirEntry *)((BYTE *)icoBuf + sizeof(IcoHeader));

	uint32_t imgOffset = sizeof(IcoHeader)
	                     + (mHeader.count * sizeof(DiskIcoDirEntry));

	uint16_t resIdx = 0;
	for (uint16_t i = 0; i < mHeader.count; i++, resIdx++) {
		// Skip over excluded entries
		// (e.g. PNGs when allowEmbeddedPNGs is set)
		if (imgDatas[resIdx] == NULL || imgDataLens[resIdx] == 0) {
			i--;
			continue;
		}

		// ResIcoDirEntry is smaller than DiskIcoDirEntry
		// Just copy the entire structure over, and then change
		// the last member which is the only differing part.
		memcpy(&diskDirEntries[i],
		       &resDirEntries[resIdx],
		       sizeof(ResIcoDirEntry));
		diskDirEntries[i].offset = imgOffset;

		// Occasionally, the icon directory entry's size field does not
		// match the resource's actual size. In at least once case
		// (Postman's exe ICO, 128x128) it was because the bitmap was >
		// 65536 bytes and it seems that even though the icon directory
		// entry has a 32 bit size field, it can only store 16 bits (it
		// comes out as 2088 instead of 67624). So, always use the
		// resource size, as it is correct.
		diskDirEntries[i].sizeBytes = imgDataLens[resIdx];

		// Copy image data from resource
		memcpy((BYTE *)icoBuf + imgOffset, imgDatas[resIdx], imgDataLens[resIdx]);

		imgOffset += imgDataLens[resIdx];
	}

	free((void *)imgDatas);
	free(imgDataLens);
	return icoBuf;
}

typedef struct {
	PBYTE icoBuf;
	DWORD bufLen;
	BOOL allowEmbeddedPNGs;
} EnumIconsData;

// Callback to enumerate RT_GROUP_ICON resources.
// Only the first one (the primary icon) is desired.
static BOOL CALLBACK enum_group_icons_callback(
	HMODULE  module,
	LPCSTR   type,
	LPSTR    name,
	LONG_PTR lpUserdata)
{
	if (lpUserdata) {
		EnumIconsData *data = (EnumIconsData *)lpUserdata;
		data->icoBuf = extract_ico_from_module(module, name, data->allowEmbeddedPNGs, &data->bufLen);
	}

	// Stop enumeration; only get the first ICO
	return FALSE;
}

PBYTE get_exe_icon_from_file_utf16(PCWSTR path, BOOL allowEmbeddedPNGs, PDWORD bufLen)
{
	if (!path || !bufLen) {
		return NULL;
	}

	HMODULE module = LoadLibraryExW(path,
		NULL,
		LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_LIBRARY_AS_DATAFILE);
	if (!module) {
		return NULL;
	}

	EnumIconsData data;
	data.icoBuf = NULL;
	data.bufLen = 0;
	data.allowEmbeddedPNGs = allowEmbeddedPNGs;

	if (!EnumResourceNamesA(module,
		(LPSTR)RT_GROUP_ICON,
		enum_group_icons_callback,
		(LONG_PTR)&data))
	{
		// Error 1813 is for when there are no RT_GROUP_ICONs.
		// Error 15106 is for "User stopped resource enumeration.",
		// which is expected behavior.
		if (GetLastError() != 15106) {
			return NULL;
		}
	}

	FreeLibrary(module);

	*bufLen = data.bufLen;
	return data.icoBuf;
}

PBYTE get_exe_icon_from_file_utf8(PCSTR path, BOOL allowEmbeddedPNGs, PDWORD bufLen)
{
	if (!path || !bufLen) {
		return NULL;
	}

	int pathBufLen = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
	if (pathBufLen <= 0) {
		return NULL;
	}

	PWSTR wPath = (PWSTR)malloc(sizeof(WCHAR) * pathBufLen);
	pathBufLen = MultiByteToWideChar(CP_UTF8, 0, path, -1, wPath, pathBufLen);
	if (pathBufLen <= 0) {
		free(wPath);
		return NULL;
	}

	PBYTE icoBuf = get_exe_icon_from_file_utf16(wPath, allowEmbeddedPNGs, bufLen);

	free(wPath);

	return icoBuf;
}

PBYTE get_exe_icon_from_handle(HANDLE process, BOOL allowEmbeddedPNGs, PDWORD bufLen)
{
	if (!process || !bufLen) {
		return NULL;
	}

	WCHAR exeNameBuf[512];
	DWORD exeNameBufLen = sizeof(exeNameBuf) / sizeof(exeNameBuf[0]);
	BOOL ret = QueryFullProcessImageNameW(process,
		0,
		exeNameBuf,
		&exeNameBufLen);
	if (!ret) {
		return NULL;
	}

	return get_exe_icon_from_file_utf16(exeNameBuf, allowEmbeddedPNGs, bufLen);
}

PBYTE get_exe_icon_from_pid(DWORD pid, BOOL allowEmbeddedPNGs, PDWORD bufLen)
{
	if (!bufLen) {
		return NULL;
	}

	HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (!process) {
		return NULL;
	}

	PBYTE icoBuf = get_exe_icon_from_handle(process, allowEmbeddedPNGs, bufLen);

	CloseHandle(process);

	return icoBuf;
}

PBYTE get_default_exe_icon(BOOL allowEmbeddedPNGs, PDWORD bufLen)
{
	if (!bufLen) {
		return NULL;
	}

	HMODULE module = LoadLibraryExA("imageres.dll",
		NULL,
		LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_LIBRARY_AS_DATAFILE);

	if (module) {
		PBYTE icoBuf = extract_ico_from_module(module, MAKEINTRESOURCEA(15), allowEmbeddedPNGs, bufLen);
		FreeLibrary(module);
		return icoBuf;
	}

	module = LoadLibraryExA("shell32.dll",
		NULL,
		LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_LIBRARY_AS_DATAFILE);
	
	if (module) {
		PBYTE icoBuf = extract_ico_from_module(module, MAKEINTRESOURCEA(3), allowEmbeddedPNGs, bufLen);
		FreeLibrary(module);
		return icoBuf;
	}

	return NULL;
}
