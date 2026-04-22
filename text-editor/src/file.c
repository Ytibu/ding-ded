#include "./file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *slurp_file(const char *filePath)
{
#define SLURP_FILE_PANIC \
    do { \
        fprintf(stderr, "Error: failed to read file '%s'\n", filePath); \
        exit(EXIT_FAILURE); \
    } while (0)

    FILE *file = fopen(filePath, "rb");
    if (!file) {
        SLURP_FILE_PANIC;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(size + 1);
    if (!buffer) {
        fclose(file);
        SLURP_FILE_PANIC;
    }

    fread(buffer, 1, size, file);
    buffer[size] = '\0';

    fclose(file);
    return buffer;
#undef SLURP_FILE_PANIC
}