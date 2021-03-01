#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>
#include "HackHelper.h"

typedef struct _FreeCamValues {
    float rotX;
    float rotY;
    float offsetX;
    float zoom;
    float offsetY;
    float lookatZ;
    float lookatX;
    float lookatY;
} FreeCamValues;

typedef struct _StaticCamValues {
    float moveZ;
    float moveX;
    float moveY;
} StaticCamValues;

int* mouseX;
int* mouseY;
int* gender;
int* age;
boolean* uiToggle;
boolean* bgToggle;
boolean* freecamToggle;
FreeCamValues* freeCam;
StaticCamValues* staticCam;

int initPointers(char* modBase);
void fixStaticCam();
void handleInput();

DWORD WINAPI MainThread(LPVOID param)
{
    boolean keepRunning = TRUE;
    char* modBase = (char*)GetModuleHandleA(NULL);

    char s[256] = { 0 };
    int code = initPointers(modBase);

    if (code != 0) {
        keepRunning = FALSE;
        MessageBoxA(
            NULL,
            "Failed to initialize one or more pointers\n"
            "Try increasing the maxRetries value in the config file",
            NULL,
            MB_OK | MB_ICONERROR
        );
    }

    while (keepRunning)
    {
        handleInput();
        fixStaticCam();

        Sleep(1);
    }

    FreeLibraryAndExitThread((HMODULE)param, 0);

    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD fwdReason, LPVOID lpReserved)
{
    if (fwdReason == DLL_PROCESS_ATTACH) {
        CreateThread(0, 0, MainThread, hModule, 0, 0);
    }
    return TRUE;
}

int initPointers(char* modBase)
{
    mouseX        = getFinalAddress(6, modBase, 0x7AB114, 0x28, 0x5C, 0x2AC, 0x8, 0x3C, 0x208);
    mouseY        = getFinalAddress(6, modBase, 0x7AB114, 0x28, 0x5C, 0x2AC, 0x8, 0x3C, 0x20C);
    age           = getFinalAddress(4, modBase, 0x7AB15C, 0x90, 0x1D8, 0x28, 0x280);
    gender        = getFinalAddress(4, modBase, 0x7AB15C, 0x90, 0x1D8, 0x28, 0x284);
    uiToggle      = getFinalAddress(6, modBase, 0x7AB114, 0x28, 0x5C, 0x2AC, 0x8, 0x3C, 0x133);
    bgToggle      = getFinalAddress(6, modBase, 0x7AB114, 0x28, 0x5C, 0x2AC, 0x8, 0x3C, 0x134);
    freecamToggle = getFinalAddress(6, modBase, 0x7AB114, 0x28, 0x5C, 0x2AC, 0x8, 0x3C, 0x135);
    freeCam       = getFinalAddress(6, modBase, 0x7AB114, 0x28, 0x5C, 0x2AC, 0x8, 0x3C, 0x138);
    staticCam     = getFinalAddress(3, modBase, 0x7BC188, 0x130, 0xA0, 0x50);

    int a = 0;
    if (mouseX == NULL) a++;
    if (mouseY == NULL) a++;
    if (age == NULL) a++;
    if (gender == NULL) a++;
    if (uiToggle == NULL) a++;
    if (bgToggle == NULL) a++;
    if (freecamToggle == NULL) a++;
    if (freeCam == NULL) a++;
    if (staticCam == NULL) a++;

    return a;
}

void handleInput()
{
    if (GetAsyncKeyState(VK_END) & 0x1) {
        *uiToggle = !(*uiToggle);
    }
}

void fixStaticCam()
{
    // No use trying to change these values in free cam mode
    // They get set somewhere else in Body Shop code
    if (freecamToggle) return;

    staticCam->moveX = 3.75f;
    staticCam->moveZ = 3.75f;
    staticCam->moveY = 1.425f;
}
