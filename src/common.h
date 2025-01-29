#ifndef COMMON_H_
#define COMMON_H_

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;

typedef float f32;
typedef double f64;

#define ARRAY_LENGTH(a) (sizeof((a)) / sizeof(((a)[0])))

#endif // COMMON_H_
