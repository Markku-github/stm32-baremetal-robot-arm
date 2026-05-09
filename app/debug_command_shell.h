#ifndef DEBUG_COMMAND_SHELL_H
#define DEBUG_COMMAND_SHELL_H

#include <stdbool.h>
#include <stdint.h>

#define DEBUG_COMMAND_SHELL_COMMAND_CAPACITY 64U

typedef struct
{
    char command_buffer[DEBUG_COMMAND_SHELL_COMMAND_CAPACITY];
    uint8_t command_length;
    bool command_overflowed;
    bool previous_byte_was_carriage_return;
} debug_command_shell_t;

typedef void (*debug_command_shell_write_string_fn)(void *context, const char *text);
typedef void (*debug_command_shell_write_byte_fn)(void *context, uint8_t byte);
typedef void (*debug_command_shell_write_prompt_fn)(void *context);
typedef void (*debug_command_shell_execute_command_fn)(void *context, const char *command_line);

typedef struct
{
    debug_command_shell_write_string_fn write_string;
    debug_command_shell_write_byte_fn write_byte;
    debug_command_shell_write_prompt_fn write_prompt;
    debug_command_shell_execute_command_fn execute_command;
} debug_command_shell_io_t;

void debug_command_shell_init(debug_command_shell_t *shell);
void debug_command_shell_reset(debug_command_shell_t *shell);
void debug_command_shell_handle_transport_overflow(
    debug_command_shell_t *shell,
    const debug_command_shell_io_t *io,
    void *context);
void debug_command_shell_handle_transport_read_error(
    debug_command_shell_t *shell,
    const debug_command_shell_io_t *io,
    void *context);
void debug_command_shell_process_byte(
    debug_command_shell_t *shell,
    uint8_t received_byte,
    const debug_command_shell_io_t *io,
    void *context);

#endif