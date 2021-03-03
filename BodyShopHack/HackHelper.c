#include <Windows.h>
#include "HackHelper.h"
#include <stdio.h>

#define CONFIG_FILE "BodyShopHack.ini"
#define MAX_RETRIES 100
#define RETRY_WAIT_MS 100

void* getFinalAddress(uint32_t n_hops, BYTE* base_addr, int maxRetries, ...)
{
    uint32_t address = 0;

    va_list valist;
    int n_tries = 0;

    while(n_tries++ < maxRetries)
    {
        address = (uint32_t)base_addr;
        va_start(valist, maxRetries);

        for (uint32_t i = 0; i < n_hops; i++)
        {
            address = *(uint32_t*)address;

            if (address == 0x0 || IsBadWritePtr((void*)address, 4)) {
                Sleep(RETRY_WAIT_MS);
                address = 0x0;
                break;
            }

            address += va_arg(valist, int);
        }
    }

    va_end(valist);

    return (void*)address;
}

BOOLEAN parseBooleanA(const char* input, BOOLEAN bDefault)
{
    // Case insensitive string compares
    if (!lstrcmpiA(input, "True")) {
        return TRUE;
    }
    else if (!lstrcmpiA(input, "False")) {
        return FALSE;
    }
    return bDefault;
}

BOOLEAN parseBooleanW(const WCHAR* input, BOOLEAN bDefault)
{
    // Case insensitive string compares
    if (!lstrcmpiW(input, L"True")) {
        return TRUE;
    }
    else if (!lstrcmpiW(input, L"False")) {
        return FALSE;
    }
    return bDefault;
}

float parseFloatW(const WCHAR* in, float fDefault)
{
    float f;
    int code;
    code = swscanf_s(in, L"%f", &f);
    if (code <= 0)
    {
        return fDefault;
    }
    return f;
}
