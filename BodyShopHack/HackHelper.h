#pragma once
#include <Windows.h>
#include <stdint.h>
#include <stdarg.h>

typedef unsigned char BOOLEAN;

enum Age {
    TODDLER = 0,
    CHILD = 1,
    TEEN = 2,
    ADULT = 3,
    ELDER = 4
};

typedef struct _AgeSettings {
    float fCamStaticBaseX;
    float fCamStaticBaseY;
    float fCamStaticBaseZ;
    float fCamFreeRotAroundX;
    float fCamFreeRotAroundY;
    float fCamFreeRotAroundZ;
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

BOOLEAN parseBoolean(const char* in, BOOLEAN bDefault);
BOOLEAN parseBooleanW(const WCHAR* in, BOOLEAN bDefault);

//void loadConfig();
