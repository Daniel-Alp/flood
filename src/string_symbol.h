#pragma once
#include <string.h>
#include "common.h"
#include "value.h"

inline u32 hash_string(const char *chars, i32 cnt)
{
    u32 hash = 2166136261u;
    for (i32 i = 0; i < cnt; i++) {
        hash ^= u8(chars[i]);
        hash *= 16777619;
    }
    return hash;
}

class String {
    i32 cnt;
    i32 cap;
    char *chars;
    u32 hash;

    String(): cnt(0), cap(8), chars(new char[cap]) {}

    ~String()
    {
        cnt = 0;
        cap = 0;
        delete[] chars;
        chars = nullptr;
        hash = 0;
    }
    
    String(const char *chars): cnt(strlen(chars)+1), cap(cnt), chars(new char[cap]), hash(hash_string(chars, cnt))
    {
        strcpy(this->chars, chars);
    };

    String(const String& other): cnt(other.cnt), cap(other.cap), chars(new char[cap]), hash(other.hash)
    {
        strcpy(chars, other.chars);
    };

    String(String&& other): cnt(other.cnt), cap(other.cap), chars(other.chars), hash(other.hash)
    {
        other.cnt = 0;
        other.cap = 0;
        other.chars = nullptr;
        other.hash = 0;
    }

    String& operator=(String other)
    {
        swap(cnt, other.cnt);
        swap(cap, other.cap);
        swap(chars, other.chars);
        swap(hash, other.hash);
        return *this;
    }

    char operator[](const i32 idx) const
    {
        return chars[idx];
    }
};
