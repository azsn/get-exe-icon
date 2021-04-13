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

#include <windows.h>

// Gets the primary icon associated with an executable, DLL, or any other file
// that LoadLibraryExW can open. The primary icon is defined by the first
// RT_GROUP_ICON resource, and is the icon shown by Explorer for executables.
// If the exe has no icon, NULL is returned. You can use get_default_exe_icon()
// to show a default icon in this case.
//
// path: A NULL-terminated UTF-16 string specifying the path of the file
//       to extract the icon from.
//
// allowEmbeddedPNGs: Set true to allow PNGs to be included within the
//                    returned ICO file, allowing for image sizes above
//                    256x256. Not all programs accept these types of ICOs.
//
// bufLen (OUT): The size of the returned buffer is written to bufLen.
//               This  must not be NULL.
//
// Return Value: An ICO file contained in a byte buffer. Free with free(3).
//               If an error occurs, NULL is returned.
PBYTE get_exe_icon_from_file_utf16(PCWSTR path, BOOL allowEmbeddedPNGs, PDWORD bufLen);

// Same as get_icon_from_file_utf16() except path is a UTF-8 string.
PBYTE get_exe_icon_from_file_utf8(PCSTR path, BOOL allowEmbeddedPNGs, PDWORD bufLen);

// Same as get_icon_from_file_utf16() except the icon is retrieved from an
// active process specified by its handle (e.g. acquired with OpenProcess).
// This simply gets the process path using QueryFullProcessImageNameW and
// then calls get_icon_from_file_utf16().
PBYTE get_exe_icon_from_handle(HANDLE process, BOOL allowEmbeddedPNGs, PDWORD bufLen);

// Same as get_icon_from_handle() except uses PID to specify a process.
PBYTE get_exe_icon_from_pid(DWORD pid, BOOL allowEmbeddedPNGs, PDWORD bufLen);

// Gets the system default executable icon from imageres.dll (Vista and up) or
// shell32.dll (XP and below). This always returns the same value, so you may
// wish to just call it once and cache the result if it's needed often. The
// parameters and return value are the same as in get_icon_from_file_utf16().
PBYTE get_default_exe_icon(BOOL allowEmbeddedPNGs, PDWORD bufLen);
