#include <Module.hpp>

MODULE_IMPL

static int64_t begin;

EXPORT_FUNC(int, add, int a, int b)
{
    return a + b;
}

EXPORT_FUNC(void, printN, int n)
{
    printf("%i\n", n);
}

EXPORT_FUNC(int, getN)
{
    int n;
    [[maybe_unused]] int _ = scanf("%i", &n);
    return n;
}
EXPORT_FUNC(void, printC, char c)
{
    printf("%c", c);
}

EXPORT_FUNC(char, getC)
{
    char c;
    scanf("%c", &c);
    return c;
}

EXPORT_FUNC(void, mwrite, long addr, char val)
{
    *(char *)addr = val;
}

EXPORT_FUNC(char, mread, long addr)
{
    return *(char *)addr;
}

EXPORT_FUNC(long, now)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() - begin;
}

EXPORT_FUNC(void, printL, long n)
{
    printf("%li\n", n);
}

EXPORT_FUNC(long, allocate, long size)
{
    return (long)malloc(size);
}

EXPORT_FUNC(void, deallocate, long addr)
{
    free((void *)addr);
}

FFI_STRUCT S
{
    int a;
    int b;
    STRUCT_NAME(S)
};

EXPORT_STRUCT(S, int, int)
