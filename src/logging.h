#ifndef LOGGING_H_
#define LOGGING_H_

#include <stddef.h>
#include <stdio.h>

/* Colour codes */
#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define CYAN "\x1b[36m"
#define BLUE "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define WHITE "\x1b[37m"
#define YELLOW "\x1b[33m"
/* Resets terminal output to normal */
#define RESET "\x1b[0m"
/* Effect codes */
#define BOLD "\x1b[1m"
#define UNDERLINE "\x1b[4m"

typedef enum {
    LOG_TYPE_ERROR = 0,
    LOG_TYPE_WARNING = 1,
    LOG_TYPE_DEBUG = 2,
    LOG_TYPE_INFO = 3,
    LOG_TYPE_NONE = 4,
} log_type_e;

// NOLINTBEGIN(*-Wincompatible-pointer-types-discards-qualifiers):
// NOTE: Suppressing lint warnings
// It keeps warning me about discarding qualifiers if I insert a string literal
#define log_err(...)                                                  \
    log_formatted_input(__FILE__, __func__, __LINE__, LOG_TYPE_ERROR, \
                        __VA_ARGS__);
#define log_warn(...)                                                   \
    log_formatted_input(__FILE__, __func__, __LINE__, LOG_TYPE_WARNING, \
                        __VA_ARGS__);

#define log_info(...)                                                \
    log_formatted_input(__FILE__, __func__, __LINE__, LOG_TYPE_INFO, \
                        __VA_ARGS__);

#define log_debug(...)                                                \
    log_formatted_input(__FILE__, __func__, __LINE__, LOG_TYPE_DEBUG, \
                        __VA_ARGS__);

#define log_none(...)                                                \
    log_formatted_input(__FILE__, __func__, __LINE__, LOG_TYPE_NONE, \
                        __VA_ARGS__);

int yama_log_init_file(FILE *file);

void log_formatted_input(const char *filename, const char *func_name,
                         size_t line_num, log_type_e log_type, char *format,
                         ...);

void log_close_file(void);
// NOLINTEND

void log_enable_color_output(int enable);

#endif  // LOGGING_H_
