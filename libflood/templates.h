#pragma once

template <typename T>
struct remove_reference {
    using type = T;
};

template <typename T>
struct remove_reference<T &> {
    using type = T;
};

template <typename T>
struct remove_reference<T &&> {
    using type = T;
};

template <typename T>
constexpr typename remove_reference<T>::type &&move(T &&t)
{
    return static_cast<typename remove_reference<T>::type &&>(t);
}

// https://stackoverflow.com/questions/42125179/c-template-argument-type-deduction
// explains why functions like alloc in arena.h work
template <typename T>
T &&forward(typename remove_reference<T>::type &t) noexcept
{
    return static_cast<T &&>(t);
}

template <class T>
void swap(T &a, T &b)
{
    T tmp(move(a));
    a = move(b);
    b = move(tmp);
}
