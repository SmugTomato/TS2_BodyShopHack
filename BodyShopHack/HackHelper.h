#pragma once
#include <stdint.h>
#include <stdarg.h>


/// <summary>
/// Returns the final address based on the specified offsets
/// </summary>
/// <param name="n_hops">The number of offsets to hop through</param>
/// <param name="base_addr">Base address of the process</param>
/// <param name="init_offset">The offset from the base to the first pointer</param>
/// <returns></returns>
void* getFinalAddress(uint32_t n_hops, char* base_addr, int init_offset, ...);
