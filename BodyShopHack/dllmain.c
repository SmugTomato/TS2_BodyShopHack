#include "dllmain.h"
#include "HackHelper.h"

#define CONFIG_FILE L"BodyShopHack.ini"
#define CFG_GLOBAL L"Global"
#define CFG_ADULT L"Adult"
#define CFG_TEEN L"Teen"
#define CFG_CHILD L"Child"
#define CFG_TODDLER L"Toddle"

int* mouseX;
int* mouseY;
int* gender;
int* age;

BOOLEAN* uiToggle;
BOOLEAN* bgToggle;
BOOLEAN* freecamToggle;

FreeCamValues* freeCam;
StaticCamValues* staticCam;
StaticCamValues staticOriginal;
StaticCamValues staticOffset;
float staticStepSize;

BYTE* uiInstructionLoc;
BYTE uiInstruction[2];

AgeSettings ageSettings[4];
BOOLEAN bUseUIFix;
BOOLEAN bDebugConsole;
int iWindowWidth;
int iWindowHeight;
int iMaxRetries = 100;

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD fwdReason, LPVOID lpReserved)
{
    if (fwdReason == DLL_PROCESS_ATTACH) {
        CreateThread(0, 0, MainThread, hModule, 0, 0);
    }
    return TRUE;
}

DWORD WINAPI MainThread(LPVOID param)
{
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);

    loadConfig();
    resizeWindow();

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
        handleInput(modBase);
        fixStaticCam();

        // Toggle UI clickability
        DWORD oldProtect;
        if (*uiToggle) {
            VirtualProtect(uiInstructionLoc, 2, PAGE_EXECUTE_READWRITE, &oldProtect);
            memcpy(uiInstructionLoc, uiInstruction, 2);
            VirtualProtect(uiInstructionLoc, 2, oldProtect, &oldProtect);
        }
        else {
            VirtualProtect(uiInstructionLoc, 2, PAGE_EXECUTE_READWRITE, &oldProtect);
            memset(uiInstructionLoc, 0x90, 2);
            VirtualProtect(uiInstructionLoc, 2, oldProtect, &oldProtect);
        }

        Sleep(1);
    }

    FreeLibraryAndExitThread((HMODULE)param, 0);

    return 0;
}

// Attempt to find main Body Shop window
HWND tryFindBSWindow()
{
    HWND hMainWnd = NULL;
    DWORD pid = GetCurrentProcessId();
    WCHAR wndClassW[1024] = { 0 };

    printf("Looking for Body Shop Window...\n");
    do
    {
        hMainWnd = FindWindowEx(NULL, hMainWnd, NULL, NULL);
        DWORD dwProcessID = 0;
        GetWindowThreadProcessId(hMainWnd, &dwProcessID);
        if (dwProcessID == pid) {
            RealGetWindowClassW(hMainWnd, wndClassW, 1024);
            if (!lstrcmpiW(wndClassW, L"{0xdb637c87}")) {
                wprintf(L"Found Window, Class: %ws, PID: %d\n", wndClassW, pid);
                break;
            }
        }
    } while (hMainWnd != NULL);

    return hMainWnd;
}

// Resize Body Shop window
void resizeWindow()
{
    HWND hMainWnd = NULL;

    while (hMainWnd == NULL)
    {
        hMainWnd = tryFindBSWindow();
        Sleep(100);
    }

    while (!IsWindowVisible(hMainWnd))
    {
        printf("Waiting for window to be visible...\n");
        Sleep(500);
    }

    if (hMainWnd != NULL)
    {

        RECT workArea;
        SystemParametersInfoA(SPI_GETWORKAREA, 0, &workArea, 0);
        int width = iWindowWidth <= workArea.right ? iWindowWidth : workArea.right;
        int height = iWindowHeight <= workArea.bottom ? iWindowHeight : workArea.bottom;

        int xPos = (workArea.right - width) / 2;
        int yPos = (workArea.bottom - height) / 2;

        SetWindowPos(hMainWnd, NULL, xPos, yPos, width, height, 0);

        printf("Resized window to %d by %d\n", width, height);
    }
    else
    {
        printf("Failed to find Body Shop window\n");
    }
}

// Dereference the pointer chains
int initPointers(char* modBase)
{
    mouseX        = getFinalAddress(6, modBase + 0x7AB114, iMaxRetries, 0x28, 0x5C, 0x2AC, 0x8, 0x3C, 0x208);
    mouseY        = getFinalAddress(6, modBase + 0x7AB114, iMaxRetries, 0x28, 0x5C, 0x2AC, 0x8, 0x3C, 0x20C);
    age           = getFinalAddress(4, modBase + 0x7AB15C, iMaxRetries, 0x90, 0x1D8, 0x28, 0x280);
    gender        = getFinalAddress(4, modBase + 0x7AB15C, iMaxRetries, 0x90, 0x1D8, 0x28, 0x284);
    uiToggle      = getFinalAddress(6, modBase + 0x7AB114, iMaxRetries, 0x28, 0x5C, 0x2AC, 0x8, 0x3C, 0x133);
    bgToggle      = getFinalAddress(6, modBase + 0x7AB114, iMaxRetries, 0x28, 0x5C, 0x2AC, 0x8, 0x3C, 0x134);
    freecamToggle = getFinalAddress(6, modBase + 0x7AB114, iMaxRetries, 0x28, 0x5C, 0x2AC, 0x8, 0x3C, 0x135);
    freeCam       = getFinalAddress(6, modBase + 0x7AB114, iMaxRetries, 0x28, 0x5C, 0x2AC, 0x8, 0x3C, 0x138);
    staticCam     = getFinalAddress(3, modBase + 0x7BC188, iMaxRetries, 0x130, 0xA0, 0x50);

    uiInstructionLoc = modBase + 0x37BEDC;
    memcpy(uiInstruction, uiInstructionLoc, 2);

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

void loadConfig()
{
    WCHAR cfgPathW[1024] = { 0 };
    GetFullPathNameW(CONFIG_FILE, 1024, cfgPathW, NULL);
    wprintf(L"%ws\n", cfgPathW);

    WCHAR buffer[1024] = { 0 };
    
    GetPrivateProfileStringW(CFG_GLOBAL, L"bDebugConsole", L"False", buffer, 1024, cfgPathW);
    bDebugConsole = parseBooleanW(buffer, FALSE);
    GetPrivateProfileStringW(CFG_GLOBAL, L"bUseUIFix", L"True", buffer, 1024, cfgPathW);
    bUseUIFix = parseBooleanW(buffer, TRUE);

    iWindowWidth = GetPrivateProfileIntW(CFG_GLOBAL, L"iWindowWidth", 1024, cfgPathW);
    iWindowHeight = GetPrivateProfileIntW(CFG_GLOBAL, L"iWindowHeight", 763, cfgPathW);
    iMaxRetries = GetPrivateProfileIntW(CFG_GLOBAL, L"iMaxRetries", 100, cfgPathW);
}

void handleInput(char* modBase)
{
    char msg[1024] = { 0 };
    if (GetAsyncKeyState(VK_END) & 0x1) {
        char* opcode = modBase + 0x37BEDC;
        sprintf_s(msg, 256, "%02hhX %02hhX", opcode[0], opcode[1]);
        MessageBoxA(NULL, msg, "Info", MB_OK);
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
        /*sprintf_s(msg, 1024, "X: %f, Z: %f, Y: %f", staticOffset.x, staticOffset.z, staticOffset.y);
        sprintf_s(msg, 1024, "%s\nX: %f, Z: %f, Y: %f", msg, staticCam->x, staticCam->z, staticCam->y);
        MessageBoxA(NULL, msg, "Static Cam Offsets", MB_OK);*/
        *uiToggle = !(*uiToggle);
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
