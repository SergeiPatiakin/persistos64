#include <stdint.h>
#include <stdbool.h>
#include "cstd.h"
#include <persistos.h>

#define KEYBOARD_COMMAND_BUFFER_LENGTH 400
#define SCRIPT_BUFFER_LENGTH 400
uint8_t *command_prompt = u8p("# ");

#define PAIR_OMS 0xB1
#define PAIR_UNQUOTED_STRING 0xB2
#define PAIR_SHELL_STMT 0xB3
#define PAIR_QUOTED_STRING 0xB4
#define PAIR_QUOTED_STRING_CONTENT 0xB5
#define PAIR_WORD_FRAGMENT 0xB6
#define PAIR_WORD 0xB7
#define PAIR_SIMPLE_COMMAND_ELEMENT 0xB8
#define PAIR_REDIRECT 0xB9
#define PAIR_REDIRECT_OPERATOR 0xBA

struct pair {
    uint64_t type;
    uint8_t *start;
    uint8_t *end;
    struct pair *first_child;
    struct pair *next_sibling;
};

// zms := ' '*
// Always returns a pair
struct pair *parse_zms(uint8_t *start, size_t length) {
    uint8_t *cur = start;
    while (*cur == ' ' && cur < start + length) cur++;
    struct pair *pair = malloc(sizeof(struct pair));
    pair->type = PAIR_OMS;
    pair->start = start;
    pair->end = cur;
    pair->first_child = NULL;
    pair->next_sibling = NULL;
    return pair;
}

// oms := ' '+
struct pair *parse_oms(uint8_t *start, size_t length) {
    uint8_t *cur = start;
    while (*cur == ' ' && cur < start + length) cur++;
    if (cur == start) {
        return NULL;
    }
    struct pair *pair = malloc(sizeof(struct pair));
    pair->type = PAIR_OMS;
    pair->start = start;
    pair->end = cur;
    pair->first_child = NULL;
    pair->next_sibling = NULL;
    return pair;
}

bool is_ordinary_character(uint8_t c) {
    return c != 0 && c != '\n' && c != '\'' && c != ' ' && c != '<' && c != '>';
}

// redirect_operator := '<' | '>'
struct pair *parse_redirect_operator(uint8_t *start, size_t length) {
    if (length == 0) {
        return NULL;
    }
    if (*start != '<' && *start != '>') {
        return NULL;
    }
    struct pair *pair = malloc(sizeof(struct pair));
    pair->type = PAIR_REDIRECT_OPERATOR;
    pair->start = start;
    pair->end = start + 1;
    pair->first_child = NULL;
    pair->next_sibling = NULL;
    return pair;
}

// unquoted_string := ordinary_char*
struct pair *parse_unquoted_string(uint8_t *start, size_t length) {
    if (length == 0 || !is_ordinary_character(*start)) {
        return NULL;
    }
    uint8_t *cur = start;
    do {
        cur++;
    } while (is_ordinary_character(*cur) && cur < start + length);
    
    struct pair *pair = malloc(sizeof(struct pair));
    pair->type = PAIR_UNQUOTED_STRING;
    pair->start = start;
    pair->end = cur;
    pair->first_child = NULL;
    pair->next_sibling = NULL;
    return pair;
}

// single_quoted_string := '\'' not_single_quote_char* '\''
struct pair *parse_single_quoted_string(uint8_t *start, size_t length) {
    if (length == 0 || *start != '\'') {
        return NULL;
    }
    uint8_t *cur = start;
    do {
        cur++;
        if (cur >= start + length) {
            return NULL;
        }
    } while (*cur != '\'');
    
    // Now *cur == '\'' and cur < start + length
    
    struct pair *content_pair = malloc(sizeof(struct pair));
    content_pair->type = PAIR_QUOTED_STRING_CONTENT;
    content_pair->start = start + 1;
    content_pair->end = cur;
    content_pair->first_child = NULL;
    content_pair->next_sibling = NULL;

    struct pair *string_pair = malloc(sizeof(struct pair));
    string_pair->type = PAIR_QUOTED_STRING;
    string_pair->start = start;
    string_pair->end = cur + 1;
    string_pair->first_child = content_pair;
    string_pair->next_sibling = NULL;
    return string_pair;
}

// word_fragment := single_quoted_string | unquoted_string
struct pair *parse_word_fragment(uint8_t *start, size_t length) {
    struct pair *child_pair = parse_single_quoted_string(start, length);
    if (!child_pair) {
        child_pair = parse_unquoted_string(start, length);
        if (!child_pair) {
            return NULL;
        }
    }
    struct pair *pair = malloc(sizeof(struct pair));
    pair->type = PAIR_WORD_FRAGMENT;
    pair->start = child_pair->start;
    pair->end = child_pair->end;
    pair->first_child = child_pair;
    pair->next_sibling = NULL;
    return pair;
}

// word := word_fragment+
struct pair *parse_word(uint8_t *start, size_t length) {
    uint8_t *cur = start;
    uint8_t *max_end = start + length;
    
    struct pair *word_fragment = parse_word_fragment(cur, max_end - cur);
    if (!word_fragment) {
        return NULL;
    }
    cur = word_fragment->end;
    struct pair *pair = malloc(sizeof(struct pair));
    pair->type = PAIR_WORD;
    pair->start = word_fragment->start;
    pair->end = word_fragment->end;
    pair->first_child = word_fragment;
    pair->next_sibling = NULL;
    
    struct pair *prev_word_fragment = word_fragment;
    while (word_fragment = parse_word_fragment(cur, max_end - cur)) {
        prev_word_fragment->next_sibling = word_fragment;
        cur = word_fragment->end;
        pair->end = word_fragment->end;
        prev_word_fragment = word_fragment;
    }
    return pair;
}

// redirect := redirect_operator zms word
struct pair *parse_redirect(uint8_t *start, size_t length) {
    uint8_t *cur = start;
    uint8_t *max_end = start + length;
    struct pair *redirect_operator_pair = parse_redirect_operator(cur, max_end - cur);
    if (!redirect_operator_pair) {
        return NULL;
    }
    cur = redirect_operator_pair->end;

    struct pair *zms_pair = parse_zms(cur, max_end - cur);
    cur = zms_pair->end;

    struct pair *word_pair = parse_word(cur, max_end - cur);
    if (!word_pair) {
        return NULL;
    }
    cur = word_pair->end;
    
    struct pair *pair = malloc(sizeof(struct pair));
    pair->type = PAIR_REDIRECT;
    pair->start = start;
    pair->end = cur;
    pair->first_child = redirect_operator_pair;
    redirect_operator_pair->next_sibling = word_pair;
    pair->next_sibling = NULL;
    return pair;
}

// simple_command_element := redirect | word
struct pair *parse_simple_command_element(uint8_t *start, size_t length) {
    struct pair *redirect_pair = parse_redirect(start, length);
    if (redirect_pair) {
        struct pair *pair = malloc(sizeof(struct pair));
        pair->type = PAIR_SIMPLE_COMMAND_ELEMENT;
        pair->start = redirect_pair->start;
        pair->end = redirect_pair->end;
        pair->first_child = redirect_pair;
        pair->next_sibling = NULL;
        return pair;
    }
    struct pair *word_pair = parse_word(start, length);
    if (word_pair) {
        struct pair *pair = malloc(sizeof(struct pair));
        pair->type = PAIR_SIMPLE_COMMAND_ELEMENT;
        pair->start = word_pair->start;
        pair->end = word_pair->end;
        pair->first_child = word_pair;
        pair->next_sibling = NULL;
        return pair;
    }
    return NULL;
}

// shell_stmt := empty | simple_command_element (zms simple_command_element)*
struct pair *parse_shell_stmt(uint8_t *start, size_t length) {
    struct pair *pair = malloc(sizeof(struct pair));
    pair->type = PAIR_SHELL_STMT;
    pair->start = start;
    pair->end = start;
    pair->first_child = NULL;
    pair->next_sibling = NULL;

    struct pair *last_sibling = NULL;
    uint8_t *cur = start;
    uint8_t *max_end = start + length;
    while (true) {
        struct pair *sce_result = parse_simple_command_element(cur, max_end - cur);
        if (!sce_result) {
            break;
        }
        cur = sce_result->end;

        if (!last_sibling) {
            last_sibling = sce_result;
            pair->first_child = last_sibling;
        } else {
            last_sibling->next_sibling = sce_result;
            last_sibling = sce_result;
        }
        pair->end = last_sibling->end;

        struct pair *oms_result = parse_zms(cur, max_end - cur);
        if (!oms_result) {
            break;
        }
        cur = oms_result->end;
        // Do not emit a struct pair for whitespace
        // last_sibling->next_sibling = oms_result;
        // last_sibling = oms_result;
    }
    return pair;
}

struct redirect_info {
    uint64_t fd;
    uint8_t target_path[SCRIPT_BUFFER_LENGTH];
};

void expand_word(struct pair *word_pair, uint8_t *out_buffer) {
    size_t exec_buffer_length = 0;
    for (struct pair *word_fragment_pair = word_pair->first_child; word_fragment_pair; word_fragment_pair = word_fragment_pair->next_sibling) {
        struct pair *child_pair = word_fragment_pair->first_child;
        if (child_pair->type == PAIR_UNQUOTED_STRING) {
            size_t child_pair_length = child_pair->end - child_pair->start;
            if (exec_buffer_length + child_pair_length >= SCRIPT_BUFFER_LENGTH) {
                fputs("shell: word is too long\n", stderr);
                exit(2);
            }
            memcpy(out_buffer + exec_buffer_length, child_pair->start, child_pair_length);
            exec_buffer_length += child_pair_length;
        } else if (child_pair->type == PAIR_QUOTED_STRING) {
            struct pair *content_pair = child_pair->first_child;
            size_t content_pair_length = content_pair->end - content_pair->start;
            if (exec_buffer_length + content_pair_length >= SCRIPT_BUFFER_LENGTH) {
                fputs("shell: word is too long\n", stderr);
                exit(2);
            }
            memcpy(out_buffer + exec_buffer_length, content_pair->start, content_pair_length);
            exec_buffer_length += content_pair_length;
        } else {
            fputs("shell: parser assertion: unexpected pair type\n", stderr);
            exit(2);
        }
        out_buffer[exec_buffer_length + 1] = 0;
    }
}

uint64_t exec_shell_line(uint8_t *start, size_t length) {
    struct pair *stmt_result = parse_shell_stmt(start, length);
    if (!stmt_result) {
        fputs(u8p("shell: syntax error\n"), stderr);
        return 1;
    }
    if (stmt_result->end != start + length) {
        fputs(u8p("shell: syntax error, unexpected '"), stderr);
        write(2, start + length, 1);
        fputs(u8p("'\n"), stderr);
        return 1;
    }

    // Count words
    uint64_t num_words = 0;
    uint64_t num_redirects = 0;
    for (struct pair *pair = stmt_result->first_child; pair; pair = pair->next_sibling) {
        if (pair->type != PAIR_SIMPLE_COMMAND_ELEMENT) {
            fputs("shell: parser assertion: expected PAIR_SIMPLE_COMMAND_ELEMENT\n", stderr);
            exit(2);
        }
        if (pair->first_child->type == PAIR_WORD) {
            num_words++;
        } else if (pair->first_child->type == PAIR_REDIRECT) {
            num_redirects++;
        }
    }

    if (num_words == 0) {
        // No-op
        free(stmt_result);
        return 0;
    }
    uint8_t **exec_argv = malloc((num_words + 1) * sizeof(void*));
    struct redirect_info *redirect_infos = malloc(num_redirects * sizeof(struct redirect_info));
    memset(redirect_infos, 0, num_redirects * sizeof(struct redirect_info));

    uint64_t word_index = 0;
    uint64_t redirect_index = 0;
    for (struct pair *sce_pair = stmt_result->first_child; sce_pair; sce_pair = sce_pair->next_sibling) {
        if (sce_pair->first_child->type == PAIR_WORD) {
            struct pair *word_pair = sce_pair->first_child;
            uint8_t *exec_buffer = malloc(SCRIPT_BUFFER_LENGTH);
            expand_word(word_pair, exec_buffer);
            exec_argv[word_index++] = exec_buffer;
        } else if (sce_pair->first_child->type == PAIR_REDIRECT) {
            struct pair *redirect_pair = sce_pair->first_child;
            struct pair *redirect_operator = redirect_pair->first_child;
            struct pair *redirect_target_word = redirect_pair->first_child->next_sibling;
            uint64_t redirect_fd;
            if (strklcmp("<", redirect_operator->start, 1) == 0) {
                redirect_fd = 0;
            } else if (strklcmp(">", redirect_operator->start, 1) == 0) {
                redirect_fd = 1;
            } else {
                fputs("shell: parser assertion: unexpected redirect operator\n", stderr);
                exit(2);
            }
            
            redirect_infos[redirect_index].fd = redirect_fd;
            expand_word(redirect_target_word, &redirect_infos[redirect_index].target_path);
            redirect_index++;
        } else {
            fputs("shell: parser assertion: unexpected pair type\n", stderr);
            exit(2);
        }
    }

    exec_argv[num_words] = NULL;

    if (strcmp(u8p("exit"), exec_argv[0]) == 0) {
        uint64_t exit_code;
        if (num_words > 1) {
            parse_n_dec(
                exec_argv[1],
                strlen(exec_argv[1]),
                &exit_code
            );
        }
        exit(exit_code);
    }

    uint64_t child_pid = fork();
    uint64_t child_exit_code;
    if (child_pid == 0) {
        for (int i = 0; i < num_redirects; i++) {
            struct redirect_info ri = redirect_infos[i];
            uint64_t old_fd = open(ri.target_path, O_CREAT | O_TRUNCATE);
            if (is_error(old_fd)) {
                fputs(u8p("shell: failed to open file for redirect\n"), stderr);
                exit(1);
            }
            uint64_t dup2_result = dup2(old_fd, ri.fd);
            if (is_error(dup2_result)) {
                fputs(u8p("shell: failed to duplicate fd"), stderr);
                exit(1);
            }
            close(old_fd);
        }

        // Try with no prefix
        exec(exec_argv[0], exec_argv);
        // Try with "bin" prefix
        uint8_t buffer[KEYBOARD_COMMAND_BUFFER_LENGTH];
        strcpy(buffer, "bin/");
        strcpy(buffer + 4, exec_argv[0]);
        exec(buffer, exec_argv);
        // Give up
        fputs(u8p("shell: command not found: "), stderr);
        fputs(exec_argv[0], stderr);
        fputs(u8p("\n"), stderr);
        exit(1);
    } else {
        if (is_error(waitpid(child_pid, &child_exit_code))) {
            fputs("shell: error waiting for subprocess\n", stderr);
        }
    }
    // write(1, pair->start, pair->end - pair->start);
    // puts(u8p("\n"));

    free(stmt_result); // TODO: recursive free
    return child_exit_code;
}

void rewrite_command_line(
    uint8_t *keyboard_command_buffer,
    uint16_t keyboard_command_length,
    uint16_t cursor_end_offset
) {
    puts(u8p("\r\x1B[K"));
    puts(command_prompt);
    write(1, keyboard_command_buffer, keyboard_command_length);
    for (uint16_t i = 0; i < cursor_end_offset; i++) {
        puts(u8p("\x1B[D"));
    }
}

int main(int argc, char* argv[]) {
    bool interactive_mode = argc < 2;
    if (interactive_mode) {
        uint16_t cursor_end_offset = 0;

        uint16_t keyboard_command_length = 0;
        uint8_t keyboard_command_buffer[KEYBOARD_COMMAND_BUFFER_LENGTH];

        uint16_t prev_keyboard_command_length = 0;
        uint8_t prev_keyboard_command_buffer[KEYBOARD_COMMAND_BUFFER_LENGTH];
        memset(keyboard_command_buffer, 0, KEYBOARD_COMMAND_BUFFER_LENGTH);
        memset(prev_keyboard_command_buffer, 0, KEYBOARD_COMMAND_BUFFER_LENGTH);

        uint16_t tmp_keyboard_command_length = 0;
        uint8_t tmp_keyboard_command_buffer[KEYBOARD_COMMAND_BUFFER_LENGTH];

        uint8_t single_char_buffer[1];
        
        puts(command_prompt);
        while (true) {
            read(0, single_char_buffer, 1);
            if (single_char_buffer[0] == 0x1B) {
                // Expect an escape code consisting of another 2 characters
                uint8_t arrow_code[2];
                read(0, arrow_code, 2);
                if (memcmp(arrow_code, "[A", 2) == 0) {
                    // Reset horizontal cursor position
                    cursor_end_offset = 0;
                    // Swap current buffer with previous buffer
                    tmp_keyboard_command_length = keyboard_command_length;
                    memcpy(tmp_keyboard_command_buffer, keyboard_command_buffer, KEYBOARD_COMMAND_BUFFER_LENGTH);
                    keyboard_command_length = prev_keyboard_command_length;
                    memcpy(keyboard_command_buffer, prev_keyboard_command_buffer, KEYBOARD_COMMAND_BUFFER_LENGTH);
                    prev_keyboard_command_length = tmp_keyboard_command_length;
                    memcpy(prev_keyboard_command_buffer, tmp_keyboard_command_buffer, KEYBOARD_COMMAND_BUFFER_LENGTH);
                    rewrite_command_line(keyboard_command_buffer, keyboard_command_length, cursor_end_offset);
                } else if (memcmp(arrow_code, "[B", 2) == 0) {
                    // Swallow arrow down
                } else if (memcmp(arrow_code, "[C", 2) == 0) {
                    // Handle arrow right
                    if (cursor_end_offset > 0) {
                        cursor_end_offset--;
                        rewrite_command_line(keyboard_command_buffer, keyboard_command_length, cursor_end_offset);
                    }
                } else if (memcmp(arrow_code, "[D", 2) == 0) {
                    // Handle arrow left
                    if (cursor_end_offset < keyboard_command_length) {
                        cursor_end_offset++;
                        rewrite_command_line(keyboard_command_buffer, keyboard_command_length, cursor_end_offset);
                    }
                } else {
                    // Swallow random escape code
                }
            } else if (single_char_buffer[0] == 0xFF) {
                // Erase previous character if it exists
                if (keyboard_command_length > cursor_end_offset) {
                    keyboard_command_length--;
                    for (uint16_t i = keyboard_command_length - cursor_end_offset; i < keyboard_command_length; i++) {
                        keyboard_command_buffer[i] = keyboard_command_buffer[i + 1];
                    }
                }
                // Rewrite command line
                rewrite_command_line(keyboard_command_buffer, keyboard_command_length, cursor_end_offset);
            } else if (single_char_buffer[0] == '\n') {
                // Print the newline
                write(1, single_char_buffer, 1);
                // Copy current command buffer to previous command buffer (minus the newline)
                memcpy(prev_keyboard_command_buffer, keyboard_command_buffer, KEYBOARD_COMMAND_BUFFER_LENGTH);
                prev_keyboard_command_length = keyboard_command_length;
                // Execute the line
                exec_shell_line(keyboard_command_buffer, keyboard_command_length);
                // Reset current command buffer
                keyboard_command_length = 0;
                memset(keyboard_command_buffer, 0, KEYBOARD_COMMAND_BUFFER_LENGTH);
                // Print prompt
                puts(command_prompt);
            } else if (cursor_end_offset > 0) {
                // Insert symbol in buffer
                keyboard_command_length++;
                for (uint16_t i = 0; i < cursor_end_offset; i++) {
                    keyboard_command_buffer[keyboard_command_length - i - 1] = keyboard_command_buffer[keyboard_command_length - i - 2];
                }
                keyboard_command_buffer[keyboard_command_length - cursor_end_offset - 1] = single_char_buffer[0];
                rewrite_command_line(keyboard_command_buffer, keyboard_command_length, cursor_end_offset);
            } else {
                // Append to the buffer
                keyboard_command_buffer[keyboard_command_length] = single_char_buffer[0];
                keyboard_command_length++;
                // Print the character with no special handling
                write(1, single_char_buffer, 1);
                continue;
            }
        }
    } else {
        size_t file_offset = 0;
        uint8_t script_buffer[SCRIPT_BUFFER_LENGTH];
        ssize_t fd = open(argv[1], 0);
        if (is_error(fd)) {
            fputs("shell: could not open script\n", stderr);
            exit(1);
        }
        while (true) {
            memset(script_buffer, 0, SCRIPT_BUFFER_LENGTH);
            ssize_t bytes_read = read(fd, script_buffer, SCRIPT_BUFFER_LENGTH);
            if (is_error(bytes_read)) {
                exit(1);
            }
            file_offset += bytes_read;
            if (bytes_read == 0) {
                break;
            }
            int line_length = -1; // Line length without newline
            for (int i = 0; i < SCRIPT_BUFFER_LENGTH && i < bytes_read; i++) {
                if (script_buffer[i] == '\n') {
                    line_length = i;
                    break;
                }
            }
            if (line_length == -1) {
                fputs("shell: incomplete line\n", stderr);
                exit(1);
            }
            // Exit on first child error
            uint64_t child_exit_code = exec_shell_line(script_buffer, line_length);
            if (child_exit_code != 0) {
                exit(child_exit_code);
            }
            size_t rewind_bytes = bytes_read - (line_length + 1);
            size_t new_offset = file_offset - rewind_bytes;
            lseek(fd, new_offset, 0);
            file_offset = new_offset;
        }
    }
    return 0;
}
