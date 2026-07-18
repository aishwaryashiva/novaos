#include "string.h"

void* memset(void* dest, int c, size_t n) {
    u8* d = (u8*)dest;
    while (n--) *d++ = (u8)c;
    return dest;
}

void* memcpy(void* dest, const void* src, size_t n) {
    u8* d = (u8*)dest;
    const u8* s = (const u8*)src;
    while (n--) *d++ = *s++;
    return dest;
}

int memcmp(const void* a, const void* b, size_t n) {
    const u8* x = (const u8*)a;
    const u8* y = (const u8*)b;
    while (n--) {
        if (*x != *y) return *x - *y;
        x++; y++;
    }
    return 0;
}

size_t strlen(const char* s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

int strcmp(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return (u8)*a - (u8)*b;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

void utoa(u32 value, char* buf, int base) {
    char tmp[32];
    int i = 0;
    if (value == 0) {
        buf[0] = '0';
        buf[1] = 0;
        return;
    }
    while (value) {
        u32 rem = value % base;
        tmp[i++] = (char)(rem < 10 ? '0' + rem : 'A' + rem - 10);
        value /= base;
    }
    int j = 0;
    while (i--) buf[j++] = tmp[i];
    buf[j] = 0;
}

void itoa(int value, char* buf, int base) {
    if (value < 0 && base == 10) {
        *buf++ = '-';
        utoa((u32)(-value), buf, base);
    } else {
        utoa((u32)value, buf, base);
    }
}
