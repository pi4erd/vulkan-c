#include "array.h"

#include <assert.h>
#include <stdlib.h>

StringArray createStringArray(size_t initialCapacity) {
    StringArray result = {
        .capacity = initialCapacity,
        .elements = (const char **)malloc(initialCapacity * sizeof(const char *)),
        .elementCount = 0
    };

    assert(result.elements != NULL); // OOM
    return result;
}

void destroyStringArray(StringArray *array) {
    free(array->elements);
    array->elements = NULL;
    array->capacity = 0;
    array->elementCount = 0;
}

void appendStringArray(StringArray *array, const char **strings, size_t count) {
    if(array->elementCount + count > array->capacity) {
        array->elements = realloc(array->elements, (array->elementCount + count) * sizeof(const char *));
        assert(array->elements != NULL); // OOM
    }

    for(size_t i = 0; i < count; i++) {
        array->elements[array->elementCount++] = strings[i];
    }
}

void addStringToArray(StringArray *array, const char *string) {
    if(array->elementCount + 1 > array->capacity) {
        array->elements = realloc(array->elements, (array->elementCount + 1) * sizeof(const char *));
        assert(array->elements != NULL); // OOM
    }

    array->elements[array->elementCount++] = string;
}

