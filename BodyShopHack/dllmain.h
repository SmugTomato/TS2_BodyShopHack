#pragma once

#include <Windows.h>
#include <memoryapi.h>
#include <Psapi.h>
#include <stdio.h>

DWORD WINAPI MainThread(LPVOID);

int initPointers(char*);

void loadConfig();

void fixStaticCam();

void handleInput(char*);

HWND tryFindBSWindow();

void resizeWindow();
