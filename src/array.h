#ifndef ARRAY_H_
#define ARRAY_H_

#include <stddef.h>

typedef struct {
    const char **elements;
    size_t elementCount;
    size_t capacity;
} StringArray;

StringArray createStringArray(size_t initialCapacity);
void destroyStringArray(StringArray *array);
void appendStringArray(StringArray *array, const char **strings, size_t count);
void addStringToArray(StringArray *array, const char *string);

#endif
