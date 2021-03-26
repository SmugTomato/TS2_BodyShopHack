#include <strsafe.h>

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
float staticStepSize = 0.01f;

BYTE* uiInstructionLoc;
BYTE uiInstruction[2];

AgeSettings ageSettings[4];
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

        WCHAR cfgPathW[1024] = { 0 };
        GetFullPathNameW(CONFIG_FILE, 1024, cfgPathW, NULL);

        WCHAR errorMessageW[2048] = { 0 };
        StringCbPrintfW(errorMessageW, 2048,
            L"%s\n%s\n\n'%s'",
            L"Failed to initialize one or more pointers, camera hack inactive.",
            L"Please try increasing the 'iMaxRetries' value in:",
            cfgPathW);

        keepRunning = FALSE;

        MessageBoxW(
            NULL,
            errorMessageW,
            L"BodyShopHack Error",
            MB_OK | MB_ICONERROR | MB_SYSTEMMODAL
        );
    }

    while (keepRunning)
    {
        __try {
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
        // Access Violation Exception tends to happen on BS Exit
        // because I can't control when it frees resources
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) {
            keepRunning = FALSE;
        }
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
        WINDOWINFO windowInfo;
        TITLEBARINFO titlebarInfo;
        int width, height;
        int borderWidth, borderHeight;

        // Get border size for X and Y
        windowInfo.cbSize = sizeof(WINDOWINFO);
        GetWindowInfo(hMainWnd, &windowInfo);
        borderWidth = 2 * windowInfo.cxWindowBorders;
        borderHeight = 2 * windowInfo.cyWindowBorders;

        // Get titlebar to add height of titlebar to desired content height
        titlebarInfo.cbSize = sizeof(TITLEBARINFO);
        GetTitleBarInfo(hMainWnd, &titlebarInfo);
        borderHeight += titlebarInfo.rcTitleBar.bottom - titlebarInfo.rcTitleBar.top;

        // Add desired content width and height to border size
        width = iWindowWidth + borderWidth;
        height = iWindowHeight + borderHeight;

        RECT workArea;
        SystemParametersInfoA(SPI_GETWORKAREA, 0, &workArea, 0);
        width = width <= workArea.right ? width : workArea.right;
        height = height <= workArea.bottom ? height : workArea.bottom;

        int xPos = (workArea.right - width) / 2;
        int yPos = (workArea.bottom - height) / 2;

        SetWindowPos(hMainWnd, NULL, xPos, yPos, width, height, 0);

        printf("Resized window to (%d, %d) for desired content size of (%d, %d)\n", width, height, iWindowWidth, iWindowHeight);
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

    // Load all age-specific options
    WCHAR section[256];
    for (int i = 0; i < 4; i++)
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
        default:
            swprintf_s(section, 256, L"Adult");
            break;
        }

        AgeSettings *as = &ageSettings[i];
        
        GetPrivateProfileStringW(section, L"fCamOffsetX", L"0", buffer, 1024, cfgPathW);
        as->fCamOffsetX = parseFloatW(buffer, 0);
        GetPrivateProfileStringW(section, L"fCamOffsetY", L"0", buffer, 1024, cfgPathW);
        as->fCamOffsetY = parseFloatW(buffer, 0);
        GetPrivateProfileStringW(section, L"fCamOffsetZ", L"0", buffer, 1024, cfgPathW);
        as->fCamOffsetZ = parseFloatW(buffer, 0);

        GetPrivateProfileStringW(section, L"fCamOffsetZoomX", L"0", buffer, 1024, cfgPathW);
        as->fCamOffsetZoomX = parseFloatW(buffer, 0);
        GetPrivateProfileStringW(section, L"fCamOffsetZoomY", L"0", buffer, 1024, cfgPathW);
        as->fCamOffsetZoomY = parseFloatW(buffer, 0);
        GetPrivateProfileStringW(section, L"fCamOffsetZoomZ", L"0", buffer, 1024, cfgPathW);
        as->fCamOffsetZoomZ = parseFloatW(buffer, 0);
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

      /*  if (isChangedOffset) {
            staticOffset.x += offsetDelta.x;
            staticOffset.y += offsetDelta.y;
            staticOffset.z += offsetDelta.z;

            staticCam->x += offsetDelta.x;
            staticCam->y += offsetDelta.y;
            staticCam->z += offsetDelta.z;

            lastFrameStaticX = staticCam->x;
            printf("StaticCam Offset: %f %f %f\n", staticOffset.x, staticOffset.y, staticOffset.z);
        }*/
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
        int curAge = 0;

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
            curAge = AGE_TODDLER;
            break;
        case AGE_CHILD:
            if ((staticValues[0]) == 0x3F666666 &&
                (staticValues[1]) == 0x3F666666 &&
                (staticValues[2]) == 0x3F919DA4)
            {
                isZoomedIn = TRUE;
                printf("Child zoomed in\n");
            }
            curAge = AGE_CHILD;
            break;
        case AGE_TEEN:
            if ((staticValues[0]) == 0x3F666666 &&
                (staticValues[1]) == 0x3F666666 &&
                (staticValues[2]) == 0x3FC7AE14)
            {
                isZoomedIn = TRUE;
                printf("Teen zoomed in\n");
            }
            curAge = AGE_TEEN;
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
            curAge = AGE_ADULT;
            break;
        }

        printf("Current Age Selected: %d\n", curAge);

        if (isZoomedIn)
        {
            staticCam->x += ageSettings[curAge].fCamOffsetZoomX;
            staticCam->y += ageSettings[curAge].fCamOffsetZoomY;
            staticCam->z += ageSettings[curAge].fCamOffsetZoomZ;
        }
        else
        {
            staticCam->x += ageSettings[curAge].fCamOffsetX;
            staticCam->y += ageSettings[curAge].fCamOffsetY;
            staticCam->z += ageSettings[curAge].fCamOffsetZ;
        }
    }

    lastFrameStaticX = staticCam->x;
}
