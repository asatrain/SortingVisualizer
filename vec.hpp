#pragma once
#include <type_traits>
#include <arena.hpp>

template<typename T>
    requires std::is_scalar_v<T>
class Vec
{
    T* base = NULL;
    int32 count = 0;
    int32 capacity = 0;

public:
    Vec() = default;
    Vec(const Vec&) = delete;
    Vec& operator=(const Vec&) = delete;

    void Init(Arena* arena, int32 count)
    {
        assert(count >= 0);

        base = (T*)arena->Push(count * sizeof(T));
        this->count = count;
        this->capacity = count;
        for(int32 i = 0; i < count; ++i)
        {
            base[i] = {};
        }
    }

    void PushBack(Arena* arena, T val)
    {
        if(count < capacity)
        {
            base[count++] = val;
        }
        else
        {
            T* old_base = base;

            int32 new_capacity = capacity > 0 ? 2 * capacity : 2;
            T* new_base = (T*)arena->Push(new_capacity * sizeof(T));
            for(int32 i = 0; i < count; ++i)
            {
                new_base[i] = old_base[i];
            }

            base = new_base;
            capacity = new_capacity;
            base[count++] = val;
        }
    }

    bool PopBack()
    {
        if(count == 0)
        {
            return false;
        }
        else
        {
            count--;
            return true;
        }
    }

    T& operator [](int32 ind)
    {
        assert(ind >= 0 && ind < count);
        return base[ind];
    }

    int32 GetCount()
    {
        return count;
    }

    const T* RawData()
    {
        return base;
    }
};
