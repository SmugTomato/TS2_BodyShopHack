#include <stdio.h>
#include <Windows.h>
#include <TlHelp32.h>
#include <string.h>
#include <stdlib.h>

#define DEFAULT_WIDTH 1280
#define DEFAULT_HEIGHT 1024
#define DEFAULT_PROC_NAME L"TS2BodyShop.exe"
#define DEFAULT_EXE_NAME "TS2BodyShop.exe"
#define DEFAULT_DLL_NAME "TS2BodyShopHack.dll"
#define MAX_STR_LEN 1024

struct ResizerProps {
	int width, height;
	const WCHAR* procName;
	const char* exeName;
};


BOOL isProcRunning(const WCHAR* procName)
{
    BOOL isFound = FALSE;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 procEntry;
        procEntry.dwSize = sizeof(procEntry);

        if (Process32First(hSnap, &procEntry)) {
            do {
                if (!_wcsicmp(procEntry.szExeFile, procName)) {
                    isFound = TRUE;
                    break;
                }
            } while (Process32Next(hSnap, &procEntry));
        }
    }
    CloseHandle(hSnap);
    return isFound;
}


HWND getWindowFromPid(DWORD pid)
{
	// find all hWnds (vhWnds) associated with a process id (dwProcessID)
	HWND hCurWnd = NULL;
	do
	{
		hCurWnd = FindWindowEx(NULL, hCurWnd, NULL, NULL);
		DWORD dwProcessID = 0;
		GetWindowThreadProcessId(hCurWnd, &dwProcessID);
		if (dwProcessID == pid) {
			return hCurWnd;
		}
	} while (hCurWnd != NULL);

	return NULL;
}


void resize(HWND hwnd, struct ResizerProps * rp)
{
	RECT workArea;
	SystemParametersInfoA(SPI_GETWORKAREA, 0, &workArea, 0);
	int width = rp->width <= workArea.right ? rp->width : workArea.right;
	int height = rp->height <= workArea.bottom ? rp->height : workArea.bottom;

	int xPos = (workArea.right - width) / 2;
	int yPos = (workArea.bottom - height) / 2;

	SetWindowPos(hwnd, NULL, xPos, yPos, width, height, 0);
}


struct ResizerProps loadProperties(const char* filename)
{
	char cBuf[MAX_STR_LEN] = { 0 };
	WCHAR wcBuf[MAX_STR_LEN] = { 0 };
	char* label = NULL;
	char* value = NULL;
	
	struct ResizerProps rp;
	rp.exeName = DEFAULT_EXE_NAME;
	rp.procName = DEFAULT_PROC_NAME;
	rp.width = DEFAULT_WIDTH;
	rp.height = DEFAULT_HEIGHT;

	FILE* f;
	fopen_s(&f, filename, "r");
	if (f == NULL) {
		printf("Couldn't find file\n");
		return rp;
	}

	int tmp;
	while (fgets(cBuf, MAX_STR_LEN, f)){
		size_t len = strlen(cBuf);
		if (len > 0 && cBuf[len - 1] == '\n') {
			cBuf[len - 1] = '\0';
		}

		label = strtok_s(cBuf, "=", &value);
		if (!_stricmp(label, "width"))
		{
			tmp = atoi(value);
			rp.width = tmp != 0 ? tmp : DEFAULT_WIDTH;
		}
		else if (!_stricmp(label, "height"))
		{
			tmp = atoi(value);
			rp.height = tmp != 0 ? tmp : DEFAULT_HEIGHT;
		}
		else if (!_stricmp(label, "exeName"))
		{
			if (!_stricmp(value, DEFAULT_EXE_NAME))
			{
				rp.exeName = malloc(MAX_STR_LEN * sizeof(char));
				strcpy_s(rp.exeName, MAX_STR_LEN, value);
			}
		}
		else if (!_stricmp(label, "procName"))
		{
			if (!_stricmp(value, DEFAULT_EXE_NAME))
			{
				rp.procName = malloc(MAX_STR_LEN * sizeof(WCHAR));
				swprintf_s(rp.procName, MAX_STR_LEN, L"%hs", value);
			}
		}
	}

	fclose(f);

	return rp;
}


int main()
{
	struct ResizerProps rp = loadProperties("resizer.cfg");

	if (isProcRunning(rp.procName)) {
		MessageBoxA(NULL, "Looks like an instance of Body Shop is already running.", "Oops!", MB_OK);
		exit(1);
	}

	HWND bodyshopHwnd = NULL;
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	CreateProcessA(
		rp.exeName,
		NULL,
		NULL,
		NULL,
		FALSE,
		CREATE_NEW_CONSOLE,
		NULL,
		NULL,
		&si,
		&pi
	);

	DWORD pid = pi.dwProcessId;
	while (bodyshopHwnd == NULL) {
		bodyshopHwnd = getWindowFromPid(pid);
		Sleep(100);
	}

	while (!IsWindowVisible(bodyshopHwnd)) {
		printf("Waiting for window to be visible...\n");
		Sleep(500);
	}

	resize(bodyshopHwnd, &rp);

	// Dll injection test
	LPVOID pszLibFileRemote = VirtualAllocEx(pi.hProcess, NULL, strlen(DEFAULT_DLL_NAME) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	if (!pszLibFileRemote == NULL) {
		WriteProcessMemory(pi.hProcess, pszLibFileRemote, DEFAULT_DLL_NAME, strlen(DEFAULT_DLL_NAME) + 1, NULL);
		HANDLE handleThread = CreateRemoteThread(pi.hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)LoadLibraryA, pszLibFileRemote, 0, NULL);
		
		if (!handleThread == NULL) {
			WaitForSingleObject(handleThread, INFINITE);
			CloseHandle(handleThread);
		}
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return 0;
}
