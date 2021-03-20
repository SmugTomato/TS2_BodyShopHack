#include <stdio.h>
#include <Windows.h>
#include <memoryapi.h>
#include <TlHelp32.h>
#include <string.h>
#include <stdlib.h>

#define DEFAULT_WIDTH 1280
#define DEFAULT_HEIGHT 1024
#define DEFAULT_PROC_NAME L"TS2BodyShop.exe"
#define CONFIG_FILE L"BodyShopHack.ini"
#define DEFAULT_EXE_NAME "TS2BodyShop.exe"
#define DEFAULT_DLL_NAME "BodyShopHack.dll"
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

int main()
{
	WCHAR cfgPathW[1024] = { 0 };
	GetFullPathNameW(CONFIG_FILE, 1024, cfgPathW, NULL);

	WCHAR procName[1024] = { 0 };
	GetPrivateProfileStringW(L"Global", L"procName", DEFAULT_PROC_NAME, procName, 1024, cfgPathW);

	if (isProcRunning(procName)) {
		MessageBoxA(NULL, "Looks like an instance of Body Shop is already running.", "Oops!", MB_OK);
		exit(1);
	}

	HWND bodyshopHwnd = NULL;
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	CreateProcessW(procName, NULL, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);

	DWORD pid = pi.dwProcessId;

	// Dll injection
	LPVOID pszLibFileRemote = VirtualAllocEx(pi.hProcess, NULL, strlen(DEFAULT_DLL_NAME) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	if (!pszLibFileRemote == 0) {
		WriteProcessMemory(pi.hProcess, pszLibFileRemote, DEFAULT_DLL_NAME, strlen(DEFAULT_DLL_NAME) + 1, NULL);
		HANDLE handleThread = CreateRemoteThread(pi.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, pszLibFileRemote, 0, NULL);
		
		if (!handleThread == 0) {
			WaitForSingleObject(handleThread, INFINITE);
			CloseHandle(handleThread);
		}
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return 0;
}
