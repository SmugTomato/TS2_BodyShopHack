#pragma once
#include <Windows.h>
#include <stdint.h>
#include <stdarg.h>

typedef unsigned char BOOLEAN;

enum Age {
    AGE_TODDLER = 0,
    AGE_CHILD = 1,
    AGE_TEEN = 2,
    AGE_ADULT = 3,
    AGE_ELDER = 4
};

typedef struct _AgeSettings {
    float fCamOffsetX;
    float fCamOffsetY;
    float fCamOffsetZ;
    float fCamOffsetZoomX;
    float fCamOffsetZoomY;
    float fCamOffsetZoomZ;
} AgeSettings;

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

// Returns the final address based on the specified offsets
void* getFinalAddress(uint32_t n_hops, BYTE* base_addr, int init_offset, ...);

BOOLEAN parseBooleanA(const char* in, BOOLEAN bDefault);
BOOLEAN parseBooleanW(const WCHAR* in, BOOLEAN bDefault);

float parseFloatW(const WCHAR* in, float fDefault);

//void loadConfig();
