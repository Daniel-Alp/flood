#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

template<class T>
void swap(T& a, T& b)
{
    T tmp(static_cast<T&&>(a));
    a = static_cast<T&&>(b);
    b = static_cast<T&&>(tmp);
}
