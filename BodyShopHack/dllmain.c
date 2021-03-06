#include "dllmain.h"
#include "HackHelper.h"

#define CONFIG_FILE L"BodyShopHack.ini"
#define CFG_GLOBAL L"Global"
#define CFG_FIXES L"Fixes"
#define CFG_ADULT L"Adult"
#define CFG_TEEN L"Teen"
#define CFG_CHILD L"Child"
#define CFG_TODDLER L"Toddler"

int* mouseX;
int* mouseY;
int* gender;
int* age;

BOOLEAN* uiToggle;
BOOLEAN* bgToggle;
BOOLEAN* freecamToggle;

FreeCamValues* freeCam;
StaticCamValues* staticCam;
StaticCamValues staticOffset;
float staticStepSize = 0.05f;

BYTE* uiInstructionLoc;
BYTE uiInstruction[2];

AgeSettings ageSettings[5];
BOOLEAN bUseUIFix;
BOOLEAN bDebugConsole;

int iWindowWidth;
int iWindowHeight;
int iMaxRetries = 100;

float lastFrameStaticX = 0;

BOOL WINAPI DllMain(HINSTANCE hModule, DWORD fwdReason, LPVOID lpReserved)
{
    if (fwdReason == DLL_PROCESS_ATTACH) {
        CreateThread(0, 0, MainThread, hModule, 0, 0);
    }
    return TRUE;
}

DWORD WINAPI MainThread(LPVOID param)
{
    loadConfig();
    resizeWindow();

    boolean keepRunning = TRUE;
    char* modBase = (char*)GetModuleHandleA(NULL);
    int code = initPointers(modBase);

    char s[256] = { 0 };

    if (code) {
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

    if (mouseX == NULL) return 1;
    if (mouseY == NULL) return 1;
    if (age == NULL) return 1;
    if (gender == NULL) return 1;
    if (uiToggle == NULL) return 1;
    if (bgToggle == NULL) return 1;
    if (freecamToggle == NULL) return 1;
    if (freeCam == NULL) return 1;
    if (staticCam == NULL) return 1;

    return 0;
}

// All done in Unicode for CJK support
void loadConfig()
{
    WCHAR cfgPathW[1024] = { 0 };
    GetFullPathNameW(CONFIG_FILE, 1024, cfgPathW, NULL);

    WCHAR buffer[1024] = { 0 };
    
    GetPrivateProfileStringW(CFG_GLOBAL, L"bDebugConsole", L"False", buffer, 1024, cfgPathW);
    bDebugConsole = parseBooleanW(buffer, FALSE);

    if (bDebugConsole)
    {
        AllocConsole();
        FILE* f;
        freopen_s(&f, "CONOUT$", "w", stdout);

        wprintf(L"Config Path: %s\n", cfgPathW);
    }

    GetPrivateProfileStringW(CFG_FIXES, L"bUseUIFix", L"True", buffer, 1024, cfgPathW);
    bUseUIFix = parseBooleanW(buffer, TRUE);

    iWindowWidth = GetPrivateProfileIntW(CFG_GLOBAL, L"iWindowWidth", 1024, cfgPathW);
    iWindowHeight = GetPrivateProfileIntW(CFG_GLOBAL, L"iWindowHeight", 768, cfgPathW);
    iMaxRetries = GetPrivateProfileIntW(CFG_GLOBAL, L"iMaxRetries", 100, cfgPathW);

    // Static cam offsets
    GetPrivateProfileStringW(CFG_GLOBAL, L"fCamStaticOffsetX", L"0", buffer, 1024, cfgPathW);
    staticOffset.x = parseFloatW(buffer, 0);
    GetPrivateProfileStringW(CFG_GLOBAL, L"fCamStaticOffsetY", L"0", buffer, 1024, cfgPathW);
    staticOffset.y = parseFloatW(buffer, 0);
    GetPrivateProfileStringW(CFG_GLOBAL, L"fCamStaticOffsetZ", L"0", buffer, 1024, cfgPathW);
    staticOffset.z = parseFloatW(buffer, 0);

    // Load all age-specific options
    WCHAR section[256];
    for (int i = 0; i < 5; i++)
    {
        switch (i)
        {
        case AGE_TODDLER:
            swprintf_s(section, 256, L"Toddler");
            break;
        case AGE_CHILD:
            swprintf_s(section, 256, L"Child");
            break;
        case AGE_TEEN:
            swprintf_s(section, 256, L"Teen");
            break;
        case AGE_ADULT:
            swprintf_s(section, 256, L"Adult");
            break;
        case AGE_ELDER:
        default:
            swprintf_s(section, 256, L"Elder");
            break;
        }

        AgeSettings *as = &ageSettings[i];
        
        GetPrivateProfileStringW(section, L"fCamStaticBaseX", L"0", buffer, 1024, cfgPathW);
        as->fCamStaticBaseX = parseFloatW(buffer, 0);
        GetPrivateProfileStringW(section, L"fCamStaticBaseY", L"0", buffer, 1024, cfgPathW);
        as->fCamStaticBaseY = parseFloatW(buffer, 0);
        GetPrivateProfileStringW(section, L"fCamStaticBaseZ", L"0", buffer, 1024, cfgPathW);
        as->fCamStaticBaseZ = parseFloatW(buffer, 0);

        GetPrivateProfileStringW(section, L"fCamRotateAroundX", L"0", buffer, 1024, cfgPathW);
        as->fCamFreeRotAroundX = parseFloatW(buffer, 0);
        GetPrivateProfileStringW(section, L"fCamRotateAroundY", L"0", buffer, 1024, cfgPathW);
        as->fCamFreeRotAroundY = parseFloatW(buffer, 0);
        GetPrivateProfileStringW(section, L"fCamRotateAroundZ", L"0", buffer, 1024, cfgPathW);
        as->fCamFreeRotAroundZ = parseFloatW(buffer, 0);
    }
}

void handleInput(char* modBase)
{
    char msg[1024] = { 0 };
    BOOLEAN isChangedOffset = FALSE;
    StaticCamValues offsetDelta = { .x = 0, .y = 0, .z = 0 };

    if (bDebugConsole)
    {
        if (GetAsyncKeyState(VK_LEFT) & 0x1) {
            offsetDelta.z += staticStepSize * 0.7f;
            offsetDelta.x -= staticStepSize * 0.7f;
            isChangedOffset = TRUE;
        }
        if (GetAsyncKeyState(VK_RIGHT) & 0x1) {
            offsetDelta.z -= staticStepSize * 0.7f;
            offsetDelta.x += staticStepSize * 0.7f;
            isChangedOffset = TRUE;
        }
        if (GetAsyncKeyState(VK_UP) & 0x1) {
            offsetDelta.z -= staticStepSize * 0.7f;
            offsetDelta.x -= staticStepSize * 0.7f;
            isChangedOffset = TRUE;
        }
        if (GetAsyncKeyState(VK_DOWN) & 0x1) {
            offsetDelta.z += staticStepSize * 0.7f;
            offsetDelta.x += staticStepSize * 0.7f;
            isChangedOffset = TRUE;
        }
        if (GetAsyncKeyState(VK_DELETE) & 0x1) {
            offsetDelta.y -= staticStepSize;
            isChangedOffset = TRUE;
        }
        if (GetAsyncKeyState(VK_INSERT) & 0x1) {
            offsetDelta.y += staticStepSize;
            isChangedOffset = TRUE;
        }

        if (isChangedOffset) {
            staticOffset.x += offsetDelta.x;
            staticOffset.y += offsetDelta.y;
            staticOffset.z += offsetDelta.z;

            staticCam->x += offsetDelta.x;
            staticCam->y += offsetDelta.y;
            staticCam->z += offsetDelta.z;

            lastFrameStaticX = staticCam->x;
            printf("StaticCam Offset: %f %f %f\n", staticOffset.x, staticOffset.y, staticOffset.z);
        }
    }
}

void fixStaticCam()
{
    // No use trying to change these values in free cam mode
    // They get set somewhere else in Body Shop code
    if (*freecamToggle) return;

    // Unfortunately no easy way to check if zoomed in besides checking staticCam values per age
    if (lastFrameStaticX != staticCam->x) {

        BOOLEAN isZoomedIn = FALSE;
        // Interpret these floats as Integers for comparison
        uint32_t* staticValues = (uint32_t*)staticCam;

        // Need to check if zoomed in
        switch (*age)
        {
        case AGE_TODDLER:
            if ((staticValues[0]) == 0x3F51EB85 &&
                (staticValues[1]) == 0x3F51EB85 &&
                (staticValues[2]) == 0x3F333333)
            {
                isZoomedIn = TRUE;
                printf("Toddler zoomed in\n");
            }
            break;
        case AGE_CHILD:
            if ((staticValues[0]) == 0x3F666666 &&
                (staticValues[1]) == 0x3F666666 &&
                (staticValues[2]) == 0x3F919DA4)
            {
                isZoomedIn = TRUE;
                printf("Child zoomed in\n");
            }
            break;
        case AGE_TEEN:
            if ((staticValues[0]) == 0x3F666666 &&
                (staticValues[1]) == 0x3F666666 &&
                (staticValues[2]) == 0x3FC7AE14)
            {
                isZoomedIn = TRUE;
                printf("Teen zoomed in\n");
            }
            break;
        case AGE_ADULT:
        case AGE_ELDER:
        default:
            if ((staticValues[0]) == 0x3F71EB85 &&
                (staticValues[1]) == 0x3F71EB85 &&
                (staticValues[2]) == 0x3FD58106)
            {
                isZoomedIn = TRUE;
                printf("Adult/Elder zoomed in\n");
            }
            break;
        }

        staticCam->x += staticOffset.x;
        staticCam->y += isZoomedIn ? 0 : staticOffset.y;
        staticCam->z += staticOffset.z;
    }

    lastFrameStaticX = staticCam->x;
}
