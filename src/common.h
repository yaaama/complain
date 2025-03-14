#ifndef COMMON_H_
#define COMMON_H_

#include <assert.h>
#include <stdbool.h>
// NOLINTBEGIN
#include <stdio.h>
// NOLINTEND

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

#ifdef NDEBUG
    #define COMPLAIN_TODO(message) ((void) 0)
    #define COMPLAIN_UNREACHABLE(message) ((void) 0)

#else
    /* If stdio is not included then lets define it */
    #if !defined(_STDIO_H_) && !defined(_STDIO_H)
int printf(const char *restrict format, ...);
int fprintf(FILE *restrict stream, const char *restrict format, ...);
    #endif

    /* Define our debugging macros */
    #define COMPLAIN_TODO(message)                                             \
        do {                                                                   \
            fprintf(stderr, "%s:%d: TODO: %s\n", __FILE__, __LINE__, message); \
            abort();                                                           \
        } while (0)
    #define COMPLAIN_UNREACHABLE(message)                                   \
        do {                                                                \
            fprintf(stderr, "%s:%d: UNREACHABLE: %s\n", __FILE__, __LINE__, \
                    message);                                               \
            abort();                                                        \
        } while (0)

#endif

void trim_trailing_ws(char *str, u64 len);

char *trim_leading_ws(char *str);

bool xis_space(char c);

u64 hash_string(const char *str);
#endif  // COMMON_H_
