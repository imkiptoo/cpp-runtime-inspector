// Test type aliases (uint8_t, int32_t, size_t, etc.)
#include <cstdint>
#include <cstddef>

int main() {
    // Fixed-width signed integers
    int8_t i8 = -128;
    int16_t i16 = -32768;
    int32_t i32 = -2147483648;
    int64_t i64 = -9223372036854775807LL;

    // Fixed-width unsigned integers
    uint8_t u8 = 255;
    uint16_t u16 = 65535;
    uint32_t u32 = 4294967295U;
    uint64_t u64 = 18446744073709551615ULL;

    // Size types
    size_t sz = sizeof(int);
    ptrdiff_t pd = 42;

    // Character types
    signed char sc = 'A';
    unsigned char uc = 'B';
    char c = 'C';

    // Floating point
    float f = 3.14f;
    double d = 2.71828;
    long double ld = 1.41421L;

    // Basic integer types
    short s = 100;
    unsigned short us = 200;
    int i = 300;
    unsigned int ui = 400;
    long l = 500;
    unsigned long ul = 600;
    long long ll = 700;
    unsigned long long ull = 800;

    return 0;
}
