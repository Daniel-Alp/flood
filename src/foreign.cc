#include "foreign.h"

template <>
Value convert_from(Value val)
{
    return val;
}

template <>
double convert_from(Value val)
{
    if (IS_NUM(val))
        return AS_NUM(val);
    throw "error: double";
}

template <>
bool convert_from(Value val)
{
    if (IS_BOOL(val))
        return AS_BOOL(val);
    throw "error: bool";
}

template <>
ListObj *convert_from(Value val)
{
    if (IS_LIST(val))
        return AS_LIST(val);
    throw "error: ListObj*";
}
