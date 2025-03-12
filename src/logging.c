
#include "logging.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define time_buff_size 16
#define max_time_buf_size 32
#define log_t_str_max_len 12

typedef struct log_context_t {
    FILE *log_file;
    int enable_color_output;
} log_context_t;

log_context_t log_opts = {0};

void get_current_time (char *buffer, int buffer_len) {
    // Get current time
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);

    // Format the time into the buffer
    size_t success = strftime(buffer, buffer_len, "%H:%M:%S", tm_now);
    if (!success) {
        perror("Failure in retrieving time.");
    }
}

void log_enable_color_output (int enable) {
    log_opts.enable_color_output = enable;
}

const char *get_log_type_color (log_type_e log_type) {
    switch (log_type) {
        case LOG_TYPE_ERROR:
            return RED;
        case LOG_TYPE_WARNING:
            return YELLOW;
        case LOG_TYPE_DEBUG:
            return CYAN;
        case LOG_TYPE_INFO:
            return GREEN;
        case LOG_TYPE_NONE:
        default:
            return RESET;
    }
}

/**
 * @brief      Opens/creates a file to append logs.
 *
 * @details    File access uses attribute `a`.
 *
 * @param      param
 *
 * @return     return type
 */
int log_init_file (const char *file_path) {

    if (file_path) {
        log_opts.log_file = fopen(file_path, "w");
        log_opts.enable_color_output = 0;
    } else {
        log_opts.log_file = stderr;
    }
    FILE *local_log_file = log_opts.log_file;

    if (local_log_file == NULL) {
        perror("Failed to open or create a log file.");
        return -1;
    }
    char timebuffer[time_buff_size];
    get_current_time(timebuffer, time_buff_size);

    int worked = fprintf(local_log_file, "===LOG AT %s===\n\n", timebuffer);
    if (worked < 0) {
        perror("Failed to print the initial header!\n");
    }
    return 0;
#undef time_buff_size
}

void log_close_file (void) {

    FILE *local_log_file = log_opts.log_file;

    if (local_log_file != NULL) {
        int errcode = fclose(local_log_file);
        if (errcode) {
            perror("Error closing file");
        }
        local_log_file = NULL;
    }
}

void log_formatted_input (const char *filename, const char *func_name,
                          size_t line_num, log_type_e log_type, char *format,
                          ...) {

    if (log_opts.log_file == NULL) {
        log_opts.log_file = stderr;
    }

    FILE *local_log_file = log_opts.log_file;

    fflush(local_log_file);

    /* Get current time */
    time_t now = time(NULL);
    struct tm *timeinfo = NULL;
    time_t time_result = time(&now);
    if (time_result == -1) {
        perror("Could not retrieve time\n");
    }
    timeinfo = localtime(&now);

    char time_buffer[max_time_buf_size];
    memset(time_buffer, 0, max_time_buf_size);
    size_t success =
        strftime(time_buffer, max_time_buf_size, "%H:%M:%S", timeinfo);
    if (!success) {
        perror("Could not write the time...\n");
    }

    char log_type_str[log_t_str_max_len];

    switch (log_type) {
        case (LOG_TYPE_INFO):
            {
                sprintf(log_type_str, "INFO");
                break;
            }
        case (LOG_TYPE_DEBUG):
            {
                sprintf(log_type_str, "DEBUG");
                break;
            }
        case (LOG_TYPE_WARNING):
            {
                sprintf(log_type_str, "WARNING");
                break;
            }
        case (LOG_TYPE_ERROR):
            {
                sprintf(log_type_str, "ERROR");
                break;
            }
        case (LOG_TYPE_NONE):
            {
                sprintf(log_type_str, "");
                break;
            }
        default:
            sprintf(log_type_str, "");
    }

    /* Get the color code */
    const char *color_code = get_log_type_color(log_type);

    // Check if color output is enabled and if output is to a terminal
    if (local_log_file == stdout || local_log_file == stderr) {
        if (&log_enable_color_output) {
            (void) fprintf(local_log_file, "%s", color_code);
        }
    }

    /* Adjusted fprintf statement */
    /* int fpr_good = fprintf(local_log_file, "%s:%zu: [%s] %s %s: ", filename,
     *                        line_num, time_buffer, log_type_str, func_name);
     */

    int fpr_good =
        fprintf(local_log_file, "(%s) %s:%zu [%s] ::: ", log_type_str, filename,
                line_num, func_name);

    if (!fpr_good) {
        perror("Could not print to file\n");
        log_close_file();
        return;
    }

    /* Print the formatting (variable args) */
    va_list args;
    va_start(args, format);
    fpr_good = vfprintf(local_log_file, format, args);
    if (fpr_good < 0) {
        perror("Variadic args error..\n");
    }
    va_end(args);

    fprintf(local_log_file, "\n");

    // Reset color if needed
    if (&log_enable_color_output) {
        fprintf(local_log_file, "%s", RESET);
    }

    fflush(local_log_file);
}
