#define WIN32_LEAN_AND_MEAN
#include "get-exe-icon.h"
#include <stdlib.h>
#include <stdio.h>

#define fatal(format, ...) do { \
	fprintf(stderr, format, ##__VA_ARGS__); \
	exit(1); \
} while (0)

void write_file(const wchar_t *path, char *buf, size_t len)
{
	FILE *f = NULL;
	if (_wfopen_s(&f, path, L"wb") || !f) {
		fatal("Cannot open file '%S'.\n", path);
	}
	if (fwrite(buf, len, 1, f) != 1) {
		fatal("Failed to write file '%S'.\n", path);
	}
	fclose(f);
}

char * read_file(const wchar_t *path, size_t *len)
{
	FILE *f = NULL;
	if (_wfopen_s(&f, path, L"rb") || !f) {
		fatal("Cannot open file '%S'.\n", path);
	}

	if (fseek(f, 0, SEEK_END)) {
		fatal("Cannot read file '%S'. (1)\n", path);
	}

	long llen = ftell(f);
	if (llen < 0) {
		fatal("Cannot read file '%S'. (2)\n", path);
	}

	if (fseek(f, 0, SEEK_SET)) {
		fatal("Cannot read file '%S'. (3)\n", path);
	}

	if (llen == 0) {
		*len = 0;
		return NULL;
	}

	char *buf = (char *)malloc(llen);
	if (fread(buf, llen, 1, f) != 1) {
		fatal("Cannot read file '%S'. (4)\n", path);
	}

	*len = llen;
	return buf;
}

void assert_bufs_equal(char *a, size_t alen, char *b, size_t blen)
{
	if (alen != blen) {
		fatal("Buf lengths dont match (a: %zd, b: %zd)\n", alen, blen);
	}

	for (size_t i = 0; i < alen; i++) {
		if (a[i] != b[i]) {
			fatal("Bufs dont match at char index %zd\n", i);
		}
	}
}

void assert_out_nonnull(char *outBuf, size_t outLen)
{
	if (outBuf == NULL || outLen == 0) {
		fatal("Failed to get icon (last error: %d)\n", GetLastError());
	}
}

void free_s(char **p)
{
	if (p == NULL || *p == NULL) {
		return;
	}
	free(*p);
	*p = NULL;
}

int main(int argc, char **argv)
{
	size_t expLen = 0;
	DWORD outLen = 0;
	char *expBuf = NULL, *outBuf = NULL;

	// Dummy exe paths. The path contains some non-ASCII characters to test
	// UTF-8 / Unicode path lookups. These exes do nothing when executed;
	// they're just there to have an icon within them.
	const char *dummyExplorerPath = "testdata\\dummyexes_\xf0\x9f\x98\xba\\dummy_exe_with_explorer_icon.exe";
	const wchar_t *dummyExplorerPathW = L"testdata\\dummyexes_\U0001f63a\\dummy_exe_with_explorer_icon.exe";
	const char *dummyWritePath = "testdata\\dummyexes_\xf0\x9f\x98\xba\\dummy_exe_with_write_icon.exe";
		
	printf("Implicitly testing get_icon_from_file_utf16 via utf8...\n");

	// ---------------
	printf("Test: get_icon_from_file_utf8 with invalid path\n");
	outBuf = get_exe_icon_from_file_utf8(NULL, TRUE, &outLen);
	if (outBuf != NULL) {
		fatal("Expected null return value.\n");
	}

	// ---------------
	printf("Test: get_icon_from_file_utf8 with invalid len\n");
	outBuf = get_exe_icon_from_file_utf8(dummyExplorerPath, TRUE, NULL);
	if (outBuf != NULL) {
		fatal("Expected null return value.\n");
	}

	// ---------------
	printf("Test: get_icon_from_file_utf8 with PNGs on icon that has PNGs\n");

	outBuf = get_exe_icon_from_file_utf8(dummyExplorerPath, TRUE, &outLen);
	assert_out_nonnull(outBuf, outLen);
	write_file(L"testdata\\explorer_out.ico", outBuf, outLen);

	expBuf = read_file(L"testdata\\explorer_expected.ico", &expLen);
	assert_bufs_equal(expBuf, expLen, outBuf, outLen);

	free_s(&outBuf);
	free_s(&expBuf);

	// ---------------
	printf("Test: get_icon_from_file_utf8 without PNGs on icon that has PNGs\n");

	outBuf = get_exe_icon_from_file_utf8(dummyExplorerPath, FALSE, &outLen);
	assert_out_nonnull(outBuf, outLen);
	write_file(L"testdata\\explorer_nopng_out.ico", outBuf, outLen);

	expBuf = read_file(L"testdata\\explorer_nopng_expected.ico", &expLen);
	assert_bufs_equal(expBuf, expLen, outBuf, outLen);

	free_s(&outBuf);
	free_s(&expBuf);

	// ---------------
	printf("Test: get_icon_from_file_utf8 with PNGs on icon that has no PNGs\n");

	outBuf = get_exe_icon_from_file_utf8(dummyWritePath, TRUE, &outLen);
	assert_out_nonnull(outBuf, outLen);
	write_file(L"testdata\\write_out.ico", outBuf, outLen);

	expBuf = read_file(L"testdata\\write_expected.ico", &expLen);
	assert_bufs_equal(expBuf, expLen, outBuf, outLen);

	free_s(&outBuf);
	free_s(&expBuf);

	// ---------------
	printf("Test: get_icon_from_file_utf8 without PNGs on icon that has no PNGs\n");

	outBuf = get_exe_icon_from_file_utf8(dummyWritePath, FALSE, &outLen);
	assert_out_nonnull(outBuf, outLen);
	write_file(L"testdata\\write_nopng_out.ico", outBuf, outLen);

	// Same expected icon as prev test
	expBuf = read_file(L"testdata\\write_expected.ico", &expLen);
	assert_bufs_equal(expBuf, expLen, outBuf, outLen);

	free_s(&outBuf);
	free_s(&expBuf);

	// ---------------
	printf("Implicitly testing get_icon_from_handle via get_icon_from_pid...\n");
	printf("Test: get_icon_from_pid\n");

	// Spawn a dummy process
	PROCESS_INFORMATION procInfo;
	STARTUPINFOW startInfo;
	ZeroMemory(&startInfo, sizeof(STARTUPINFOW));
	ZeroMemory(&procInfo, sizeof(PROCESS_INFORMATION));
	startInfo.cb = sizeof(STARTUPINFO);
	if (CreateProcessW(dummyExplorerPathW, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &startInfo, &procInfo) == 0) {
		fatal("Failed to create dummy process (error: %d)", GetLastError());
	}

	// pid is valid until procInfo.hProcess is closed
	outBuf = get_exe_icon_from_pid(procInfo.dwProcessId, TRUE, &outLen);
	assert_out_nonnull(outBuf, outLen);
	write_file(L"testdata\\pid_test_out.ico", outBuf, outLen);

	expBuf = read_file(L"testdata\\explorer_expected.ico", &expLen);
	assert_bufs_equal(expBuf, expLen, outBuf, outLen);

	free_s(&outBuf);
	free_s(&expBuf);

	CloseHandle(procInfo.hProcess);
	CloseHandle(procInfo.hThread);

	// ---------------
	printf("Test: get_icon_from_pid on own process should return NULL (no icon)\n");
	outBuf = get_exe_icon_from_pid(GetProcessId(NULL), TRUE, &outLen);
	if (outBuf != NULL) {
		fatal("Expected no icon to be found.\n");
	}

	// ---------------
	printf("Test: get_default_exe_icon\n");
	outBuf = get_default_exe_icon(TRUE, &outLen);
	assert_out_nonnull(outBuf, outLen);
	write_file(L"testdata\\default_exe_out.ico", outBuf, outLen);

	HMODULE module = LoadLibraryExA("imageres.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_LIBRARY_AS_DATAFILE);
	if (module) { // Vista and up
		expBuf = read_file(L"testdata\\default_exe_imageres_expected.ico", &expLen);
		assert_bufs_equal(expBuf, expLen, outBuf, outLen);
	} else { // XP and below
		expBuf = read_file(L"testdata\\default_exe_shell32_expected.ico", &expLen);
		assert_bufs_equal(expBuf, expLen, outBuf, outLen);
	}

	free_s(&outBuf);
	free_s(&expBuf);

	printf("All tests passed\n");
	return 0;
}
