#include "file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *slup_file(const char *filePath, size_t *out_size)
{
    FILE *file = fopen(filePath, "rb");
    if (!file) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, size, file);
    buffer[size] = '\0';

    fclose(file);

    if (out_size) {
        *out_size = size;
    }
    return buffer;
}