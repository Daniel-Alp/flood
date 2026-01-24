#pragma once
#include "arena.h"
#include "gc.h"
#include "object.h"
#include "fl_string.h"
#include "value.h"
#include "vm.h"

// TODO other objects (specifically ObjInstance * and some others)

template <typename T, typename U>
constexpr bool is_same = false;

template <typename T>
constexpr bool is_same<T, T> = true;

template <typename T>
constexpr bool is_foreign_fn_arg = (is_same<T, Value>            // unchecked Value
                                    || is_same<T, double>        // VAL_NUM
                                    || is_same<T, bool>          // VAL_BOOL
                                    || is_same<T, ListObj *>     // VAL_OBJ, OBJ_LIST
                                    || is_same<T, StringObj *>); // VAL_OBJ, OBJ_STRING

template <typename>
constexpr bool is_foreign_fn = false;

template <typename R, typename... Args>
constexpr bool is_foreign_fn<R (*)(Args...)> =
    (is_foreign_fn_arg<R> || is_same<R, void>) && (is_foreign_fn_arg<Args> && ...);

template <typename>
struct FunctionArity;

template <typename R, typename... Args>
struct FunctionArity<R (*)(Args...)> {
    static constexpr int value = sizeof...(Args);
};

template <int... I>
struct IdxSeq {};

template <int N, int... I>
struct MakeIdxSeq : MakeIdxSeq<N - 1, N - 1, I...> {};

template <int... I>
struct MakeIdxSeq<0, I...> {
    using type = IdxSeq<I...>;
};

template <typename T>
T convert_from(Value val);
template <>
Value convert_from(Value val);
template <>
double convert_from(Value val);
template <>
bool convert_from(Value val);
template <>
ListObj *convert_from(Value val);
template <>
StringObj *convert_from(Value val);

inline Value convert_to(Value val)
{
    return val;
}

inline Value convert_to(double val)
{
    return MK_NUM(val);
}

inline Value convert_to(bool val)
{
    return MK_BOOL(val);
}

inline Value convert_to(Obj *val)
{
    return MK_OBJ(val);
}

template <auto F, typename R, typename... Args, int... I>
Value foreign_fn_call(Value *vals, R (*)(Args...), IdxSeq<I...>)
{
    if constexpr (is_same<R, void>) {
        F(convert_from<Args>(vals[I])...);
        return MK_NULL;
    } else {
        return convert_to(F(convert_from<Args>(vals[I])...));
    }
}

template <auto F>
InterpResult wrap_foreign_fn(Value *vals)
{
    constexpr int N = FunctionArity<decltype(F)>::value;
    using Idxs = typename MakeIdxSeq<N>::type;
    try {
        return InterpResult{.tag = INTERP_OK, .val = foreign_fn_call<F>(vals, F, Idxs{})};
    } catch (const char *message) {
        return InterpResult{.tag = INTERP_ERR, .message = message};
    }
}

template <auto F>
    requires is_foreign_fn<decltype(F)>
void define_foreign_method(VM &vm, ClassObj &klass, const String &name)
{
    constexpr int N = FunctionArity<decltype(F)>::value;
    StringObj *string = alloc<StringObj>(vm, name);
    ForeignFnObj *f_fn = alloc<ForeignFnObj>(vm, string, wrap_foreign_fn<F>, N);
    klass.methods.insert(*string, MK_OBJ(f_fn));
}
