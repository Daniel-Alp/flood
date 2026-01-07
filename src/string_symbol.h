#pragma once
#include "common.h"
#include "dynarr.h" // consider putting slice into its own thing
#include "scan.h"
#include <string.h>

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
    ~String()
    {
        cnt = 0;
        cap = 0;
        delete[] chars_;
        chars_ = nullptr;
        hash_ = 0;
    }

    String(const char *chars) : cnt(strlen(chars) + 1), cap(cnt), chars_(new char[cap]), hash_(hash_string(chars, cnt))
    {
        strcpy(this->chars_, chars);
    };

    String(const Span span)
        : cnt(span.len + 1), cap(cnt), chars_(new char[cap]), hash_(hash_string(span.start, span.len))
    {
        strncpy(chars_, span.start, span.len);
        chars_[span.len] = '\0';
    }

    String(const String &other) : cnt(other.cnt), cap(other.cap), chars_(new char[cap]), hash_(other.hash_)
    {
        strcpy(chars_, other.chars_);
    };

    String(String &&other) : cnt(other.cnt), cap(other.cap), chars_(other.chars_), hash_(other.hash_)
    {
        other.cnt = 0;
        other.cap = 0;
        other.chars_ = nullptr;
        other.hash_ = 0;
    }

    String &operator=(String other)
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

    i32 len() const
    {
        return cnt - 1;
    }

    char operator[](const i32 idx) const
    {
        return chars_[idx];
    }

    const char *chars() const
    {
        return chars_;
    }

    u32 hash() const
    {
        return hash_;
    }
};
