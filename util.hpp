#pragma once
#include <cstdint>

#define int32 int32_t
#define uint32 uint32_t
#define int64 int64_t
#define uint64 uint64_t

#define wchar wchar_t

#ifdef _DEBUG 
#define assert(cond) do { if(!(cond)) __debugbreak(); } while(0)
#else
#define assert(cond) do {} while(0)
#endif

static void swap(int32* a, int32* b)
{
    int32 t = *a;
    *a = *b;
    *b = t;
}
