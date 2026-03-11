#pragma once
#include <util.hpp>

struct Arena
{
    void* base;
    uint32 used;
    uint32 capacity;

    void* Push(uint32 amount)
    {
        assert(used + amount <= capacity);

        void* res = (char*)base + used;
        used += amount;
        return res;
    }
};
