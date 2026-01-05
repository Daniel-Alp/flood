#pragma once
#include <string.h>
#include "common.h"
#include "dynarr.h" // consider putting slice into its own thing

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
    char *chars_;
    u32 hash_;
public:
    String(): cnt(0), cap(8), chars_(new char[cap]) {}

    ~String()
    {
        cnt = 0;
        cap = 0;
        delete[] chars_;
        chars_ = nullptr;
        hash_ = 0;
    }
    
    String(const char *chars): cnt(strlen(chars)+1), cap(cnt), chars_(new char[cap]), hash_(hash_string(chars, cnt))
    {
        strcpy(this->chars_, chars);
    };

    String(const String& other): cnt(other.cnt), cap(other.cap), chars_(new char[cap]), hash_(other.hash_)
    {
        strcpy(chars_, other.chars_);
    };

    String(String&& other): cnt(other.cnt), cap(other.cap), chars_(other.chars_), hash_(other.hash_)
    {
        other.cnt = 0;
        other.cap = 0;
        other.chars_ = nullptr;
        other.hash_ = 0;
    }

    String& operator=(String other)
    {
        swap(cnt, other.cnt);
        swap(cap, other.cap);
        swap(chars_, other.chars_);
        swap(hash_, other.hash_);
        return *this;
    }

    bool operator==(const String &other) const
    {
        return hash_ == other.hash_ && cnt == other.cnt && memcmp(chars_, other.chars_, cnt) == 0;
    }

    char operator[](const i32 idx) const
    {
        return chars_[idx];
    }

    i32 size() const
    {
        return cnt;
    }

    const char *chars() const
    {
        return chars_;
    }

    u32 hash() const
    {
        return hash_;
    }

    Slice<char> slice()
    {
        return Slice<char>(chars_, cnt);
    }
};
