#include <Windows.h>
#include "HackHelper.h"

#define MAX_RETRIES 100
#define RETRY_WAIT_MS 50

void* getFinalAddress(uint32_t n_hops, char* base_addr, int init_offset, ...)
{
    char* ptr;
    ptr = base_addr + init_offset;
    if (ptr == NULL) {
        return NULL;
    }

    va_list valist;
    va_start(valist, init_offset);
    int n_tries = 0;

    while (n_tries++ < MAX_RETRIES)
    {
        ptr = base_addr + init_offset;

        for (uint32_t i = 0; i < n_hops; i++)
        {
            ptr = (char*)(*(uint32_t*)ptr) + va_arg(valist, int);

            if (ptr == NULL) {
                break;
            }
        }

        if (ptr != NULL) {
            break;
        }

        Sleep(RETRY_WAIT_MS);
    }

    va_end(valist);

    return ptr;
}
