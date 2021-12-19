#include <Module.hpp>
MODULE_IMPL

FFI_STRUCT S
{
    int a;
    STRUCT_NAME(S)
};
EXPORT_STRUCT(S, int)

EXPORT_FUNC(int, stuff, int a, int b, int c)
{
    return a + b * c;
}