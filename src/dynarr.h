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
                new (new_vals + i) T(move(vals[i]));
            }
            operator delete(vals);
            vals = new_vals;
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

    Dynarr(const Dynarr &other): cnt(0), cap(other.len()), vals(static_cast<T*>(operator new(cap * sizeof(T))))
    {
        for (i32 i = 0; i < other.len(); i++) {
            push(other[i]);
        }
    }

    Dynarr(Dynarr &&other): cnt(other.cnt), cap(other.cap), vals(other.vals)
    {
        other.cnt = 0;
        other.cap = 0;
        other.vals = nullptr;
    }

    Dynarr& operator=(Dynarr other)
    {
        swap(cnt, other.cnt);
        swap(cap, other.cap);
        swap(vals, other.vals);
        return *this;
    }
    
    void push(const T &e)
    {
        grow();
        new (vals + cnt) T(e);
        cnt++;
    };
    
    void push(T &&e)
    {
        grow();
        new (vals + cnt) T(move(e));
        cnt++;
    };
    
    i32 len() const
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
