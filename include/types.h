#ifndef NOVA_TYPES_H
#define NOVA_TYPES_H

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef signed char        i8;
typedef signed short       i16;
typedef signed int         i32;
typedef u32                size_t;
typedef u8                 bool;

#define true  1
#define false 0
#define NULL  ((void*)0)

typedef struct {
    u16 width;
    u16 height;
    u16 bpp;
    u16 pitch;
    u32 framebuffer;
} boot_info_t;

#endif
