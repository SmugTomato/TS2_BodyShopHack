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
    float rotAroundZ;
    float rotAroundX;
    float rotAroundY;
} FreeCamValues;

typedef struct _StaticCamValues {
    float z;
    float x;
    float y;
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
StaticCamValues staticOriginal;
StaticCamValues staticOffset;
float staticStepSize;

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

    staticOffset.x = 0;
    staticOffset.z = 0;
    staticOffset.y = 0;
    staticStepSize = 0.05f;

    memcpy(&staticOriginal, staticCam, 3 * sizeof(float));

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
    char msg[1024] = { 0 };
    if (GetAsyncKeyState(VK_END) & 0x1) {
        *uiToggle = !(*uiToggle);
    }
    if (GetAsyncKeyState(VK_LEFT) & 0x1) {
        staticOffset.z += staticStepSize * 0.7f;
        staticOffset.x -= staticStepSize * 0.7f;
    }
    if (GetAsyncKeyState(VK_RIGHT) & 0x1) {
        staticOffset.z -= staticStepSize * 0.7f;
        staticOffset.x += staticStepSize * 0.7f;
    }
    if (GetAsyncKeyState(VK_UP) & 0x1) {
        staticOffset.z -= staticStepSize * 0.7f;
        staticOffset.x -= staticStepSize * 0.7f;
    }
    if (GetAsyncKeyState(VK_DOWN) & 0x1) {
        staticOffset.z += staticStepSize * 0.7f;
        staticOffset.x += staticStepSize * 0.7f;
    }
    if (GetAsyncKeyState(VK_DELETE) & 0x1) {
        staticOffset.y -= staticStepSize;
    }
    if (GetAsyncKeyState(VK_INSERT) & 0x1) {
        staticOffset.y += staticStepSize;
    }
    if (GetAsyncKeyState(VK_HOME) & 0x1) {
        sprintf_s(msg, 1024, "X: %f, Z: %f, Y: %f", staticOffset.x, staticOffset.z, staticOffset.y);
        sprintf_s(msg, 1024, "%s\nX: %f, Z: %f, Y: %f", msg, staticCam->x, staticCam->z, staticCam->y);
        MessageBoxA(NULL, msg, "Static Cam Offsets", MB_OK);
    }
}

void fixStaticCam()
{
    // No use trying to change these values in free cam mode
    // They get set somewhere else in Body Shop code
    //if (freecamToggle) return;

    staticCam->x = staticOriginal.x + staticOffset.x;
    staticCam->z = staticOriginal.z + staticOffset.z;
    staticCam->y = staticOriginal.y + staticOffset.y;
}
