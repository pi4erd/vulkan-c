#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void printUsage(const char *program);
size_t formatBytes(char *buffer, size_t bufferSize, const char *filename, const char *bytes, size_t byteCount);
void writeFile(const char *filename, const char *buffer, size_t size);
char *readBytes(const char *filename, size_t *size);

void printUsage(const char *program) {
    fprintf(stderr, "Usage of %s: %s <input> [-o output]\n", program, program);
}

size_t formatBytes(char *buffer, size_t bufferSize, const char *filename, const char *bytes, size_t byteCount) {
    size_t filename_len = strlen(filename);

    char *filename_lower = malloc(filename_len + 1);
    char *filename_upper = malloc(filename_len + 1);
    char *bytearray_str = (char*)malloc(6 * byteCount + 1);

    memcpy(filename_lower, filename, filename_len + 1);
    memcpy(filename_upper, filename, filename_len + 1);

    const char availableChars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    for(size_t i = 0; i < filename_len; i++) {
        // Check if we can use the character
        if (strchr(availableChars, filename[i]) == NULL) {
            filename_lower[i] = '_';
            filename_upper[i] = '_';
        } else {
            filename_lower[i] = tolower(filename_lower[i]);
            filename_upper[i] = toupper(filename_upper[i]);
        }
    }
    
    const char hex[] = "0123456789ABCDEF";

    for(size_t i = 0; i < byteCount; i++) {
        // Total 6 characters per byte
        bytearray_str[i * 6 + 0] = '0';
        bytearray_str[i * 6 + 1] = 'x';
        bytearray_str[i * 6 + 2] = hex[(bytes[i] & 0xF0) >> 4];
        bytearray_str[i * 6 + 3] = hex[bytes[i] & 0x0F];
        bytearray_str[i * 6 + 4] = ',';
        bytearray_str[i * 6 + 5] = ' ';
    }

    bytearray_str[byteCount * 6] = '\0';

    size_t written = snprintf(buffer, bufferSize,
        "#ifndef %s\n"
        "#define %s\n\n"
        "const unsigned char %s[] = {\n"
        "    %s\n"
        "};\n"
        "\n#endif\n", 
        filename_upper,
        filename_upper,
        filename_lower,
        bytearray_str
    );

    free(filename_lower);
    free(filename_upper);
    free(bytearray_str);

    return written;
}

void writeFile(const char *filename, const char *buffer, size_t size) {
    FILE *file = fopen(filename, "w");

    if(file == NULL) {
        fprintf(stderr, "Could not open file for writing: \"%s\"\n", filename);
        exit(1);
    }

    fwrite(buffer, size, 1, file);
    fclose(file);
}

char *readBytes(const char *filename, size_t *size) {
    FILE *file = fopen(filename, "r");

    if(file == NULL) {
        fprintf(stderr, "No file or directory \"%s\"\n", filename);
        return NULL;
    }
    
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    *size = fileSize;
    fseek(file, 0L, SEEK_SET);

    char *result = (char*)malloc(fileSize);

    fread(result, fileSize, 1, file);
    fclose(file);

    return result;
}

int main(int argc, char **argv) {
    const char *program = argv[0];

    if(argc <= 1) {
        printUsage(program);
        return 1;
    }

    char *input_file, *output_file = "out.h";

    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-o") == 0) {
            assert(++i < argc && "No argument after -o");
            output_file = argv[i];
            continue;
        }

        input_file = argv[i];
    }

    size_t size;
    char *bytes = readBytes(input_file, &size);

    if(bytes == NULL) {
        return 1;
    }

    // "HEADER\n 0xhbytelbyte, ...\nFOOTER"

    size_t bufferSize = 4097 + size * 6;
    char *buffer = malloc(bufferSize);

    size_t written = formatBytes(buffer, bufferSize, output_file, bytes, size);

    writeFile(output_file, buffer, written);

    free(buffer);
    free(bytes);

    return 0;
}
