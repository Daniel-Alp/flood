#pragma once
#include <new>
#include "common.h"

template<typename T>
class Dynarr
{
    i32 cnt;
    i32 cap;
    T *vals;

    void grow()
    {
        if (cnt == cap) {
            T *new_vals = static_cast<T*>(operator new(cap*2 * sizeof(T)));
            for (i32 i = 0; i < cap; i++) {
                new (new_vals + i) T(vals[i]);
            }
            operator delete(vals);
            new_vals = vals;
            cap *= 2;
        }
    }

public:
    Dynarr(): cnt(0), cap(8), vals(static_cast<T*>(operator new(cap * sizeof(T)))) {};

    ~Dynarr()
    {
        for (i32 i = 0; i < cnt; i++) {
            vals[i].~T();
        }
        operator delete(vals);
        vals = nullptr;
        cnt = 0;
        cap = 0;
    };
    
    void push(const T &e)
    {
        grow();
        new (vals + cnt) T(e);
        cnt++;
    };
    
    void push(T &&e)
    {
        grow();
        new (vals + cnt) T(static_cast<T&&>(e));
        cnt++;
    };
    
    i32 size() const
    {
        return cnt;
    };

    T& operator[](const i32 idx) 
    {
        return vals[idx];
    };
    
    const T& operator[](const i32 idx) const
    {
        return vals[idx];
    };
};
