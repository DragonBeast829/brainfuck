#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// String
typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} String;

String String_new(size_t initial_capacity) {
    String s;
    s.data     = malloc(initial_capacity);
    s.length   = 0;
    s.capacity = initial_capacity;
    s.data[0]  = '\0';
    
    return s;
}

void String_append(String *s, const char *text) {
    size_t text_length  = strlen(text);
    size_t total_length = s->length + text_length + 1;
    
    // Resize the string if necessary
    if (s->capacity < total_length) {
        size_t new_capacity = s->capacity * 2;
        if (new_capacity < total_length) {
            new_capacity = total_length;
        }
        s->data     = realloc(s->data, new_capacity);
        s->capacity = new_capacity;
    }
    
    // Append the new text
    memcpy(s->data + s->length, text, text_length);
    s->length += text_length;
    s->data[s->length] = '\0';
}

String String_from(const char *initial_data) {
    String s = String_new(strlen(initial_data) + 1);
    String_append(&s, initial_data);
    return s;
}

void String_append_string(String *s, String text) {
    size_t total_length = s->length + text.length + 1;
    
    // Resize the string if necessary
    if (s->capacity < total_length) {
        size_t new_capacity = s->capacity * 2;
        if (new_capacity < total_length) {
            new_capacity = total_length;
        }
        s->data     = realloc(s->data, new_capacity);
        s->capacity = new_capacity;
    }
    
    // Append the new text
    memcpy(s->data + s->length, text.data, text.length);
    s->length += text.length;
    s->data[s->length] = '\0';
}

String String_format(const char *format, ...) {
    va_list args;
    va_start(args, format);

    // get required length
    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);

    if (needed < 0) {
        va_end(args);
        return String_new(4); // formatting error
    }

    // allocate temporary buffer
    char *buffer = malloc(needed + 1);
    if (!buffer) {
        va_end(args);
        return String_new(4); // allocation error
    }
    
    vsnprintf(buffer, needed + 1, format, args);
    va_end(args);

    String s = String_from(buffer);

    free(buffer);
    return s;
}

void String_free(String *s) {
    free(s->data);
    s->data = NULL;
    s->length = s->capacity = 0;
}

String to_c(const char *source, int tape_size) {
    String result = String_format(
        "#include <stdio.h>\nint main(){int tape[%d]={0};int pos = 0;",
        tape_size
    );
    int loop_depth = 0;
    size_t source_length = strlen(source);
    for (const char *ptr = source; *ptr != '\0'; ptr++) {
        char c = *ptr;
        switch (c) {
            case '>': {
                int count = 1;
                while (*(ptr + 1) == '>') {
                    count++;
                    ptr++;
                }
                String_append_string(&result, String_format("pos += %d;", count));
                break;
            }
            case '<': {
                int count = 1;
                while (*(ptr + 1) == '<') {
                    count++;
                    ptr++;
                }
                String_append_string(&result, String_format("pos -= %d;", count));
                break;
            }
            case '+': {
                int count = 1;
                while (*(ptr + 1) == '+') {
                    count++;
                    ptr++;
                }
                String_append_string(&result, String_format("tape[pos] += %d;", count));
                break;
            }
            case '-': {
                int count = 1;
                while (*(ptr + 1) == '-') {
                    count++;
                    ptr++;
                }
                String_append_string(&result, String_format("tape[pos] -= %d;", count));
                break;
            }
            case '.': {
                String_append(&result, "printf(\"%c\", tape[pos]);");
                break;
            }
            case ',': {
                String_append(&result, "tape[pos] = getchar();");
                break;
            }
            case '[': {
                String_append(&result, "while(tape[pos] != 0){");
                loop_depth++;
                break;
            }
            case ']': {
                if (loop_depth > 0) {
                    String_append(&result, "}");
                    loop_depth--;
                }
                break;
            }
        }
    }
    String_append(&result, "\n}");
    return result;
}

char* read_file_to_string(const char* filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        return NULL;
    }

    // move to the end of the file to get its size
    if (fseek(file, 0, SEEK_END) != 0) {
        perror("Failed to seek file");
        fclose(file);
        return NULL;
    }

    long file_size = ftell(file); // get file size
    if (file_size == -1) {
        perror("Failed to get file size");
        fclose(file);
        return NULL;
    }

    rewind(file); // go back to the beginning

    // allocate memory for the string
    char *buffer = malloc(file_size + 1);
    if (!buffer) {
        perror("Failed to allocate memory");
        fclose(file);
        return NULL;
    }

    // Read the file into the buffer
    size_t read_size = fread(buffer, 1, file_size, file);
    if (read_size != file_size) {
        perror("Failed to read file");
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[file_size] = '\0';

    fclose(file);
    return buffer;
}

int main(int argc, char *argv[]) {
    char *input_file_name;
    char *output_file_name;

    if (argc == 1) {
        // Not enough arguments supplied
        printf("Not enough arguments supplied.\n");
        return 1;
    } else if (argc == 2) {
        // Only input file provided; generate default output file name
        input_file_name = argv[1];
        // We'll generate a temporary C file and default executable name
        output_file_name = "a.exe";
    } else {
        // Input and output file names provided
        input_file_name = argv[1];
        output_file_name = argv[2];
    }

    // Read Brainfuck source into a string
    char *input = read_file_to_string(input_file_name);
    if (!input) {
        return 1;
    }

    // generate c
    String output = to_c(input, 3000);

    char c_file_name[1024];
    snprintf(c_file_name, sizeof(c_file_name), "%s.c", input_file_name);

    // write generated c to file
    FILE *output_file = fopen(c_file_name, "w");
    if (!output_file) {
        perror("Error creating output C file");
        String_free(&output);
        free(input);
        return 1;
    }
    fprintf(output_file, "%s", output.data);
    fclose(output_file);

    // compile
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "gcc %s -o %s", c_file_name, output_file_name);
    system(cmd);

    // remove the temporary c file
    remove(c_file_name);

    String_free(&output);
    free(input);
    return 0;
}
