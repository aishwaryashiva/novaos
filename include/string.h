#ifndef NOVA_STRING_H
#define NOVA_STRING_H

#include "types.h"

void* memset(void* dest, int c, size_t n);
void* memcpy(void* dest, const void* src, size_t n);
int memcmp(const void* a, const void* b, size_t n);
size_t strlen(const char* s);
int strcmp(const char* a, const char* b);
char* strcpy(char* dest, const char* src);
void itoa(int value, char* buf, int base);
void utoa(u32 value, char* buf, int base);

#endif
