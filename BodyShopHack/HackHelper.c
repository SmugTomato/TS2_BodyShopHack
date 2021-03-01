#include <Windows.h>
#include "HackHelper.h"

#define MAX_RETRIES 100
#define RETRY_WAIT_MS 100

void* getFinalAddress(uint32_t n_hops, char* base_addr, int init_offset, ...)
{
    uint32_t address;

    va_list valist;
    int n_tries = 0;

    while(n_tries++ < MAX_RETRIES)
    {
        address = (uint32_t)base_addr + init_offset;
        va_start(valist, init_offset);

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
