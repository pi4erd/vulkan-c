#ifndef ARRAY_H_
#define ARRAY_H_

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#define DEFINE_ARRAY(NAME, TYPE) typedef struct { \
        TYPE *elements; \
        size_t elementCount; \
        size_t capacity; \
    } NAME ## Array; \
    static NAME##Array NAME##ArrayNew(size_t initialCapacity); \
    static void NAME##ArrayDestroy(NAME ## Array *array); \
    static void NAME##ArrayAddElement(NAME ## Array *array, TYPE element); \
    static void NAME##ArrayAppendConstArray(NAME ## Array *array, TYPE *other, size_t count); \
    inline NAME##Array NAME##ArrayNew(size_t initialCapacity) { \
        NAME ## Array result = { \
            .elements = (TYPE*)malloc(initialCapacity * sizeof(TYPE)), \
            .elementCount = 0, \
            .capacity = initialCapacity, \
        }; \
        \
        assert(result.elements != NULL); \
        return result; \
    } \
    inline void NAME##ArrayDestroy(NAME ## Array *array) { \
        free(array->elements); \
        array->elements = NULL; \
        array->capacity = 0; \
        array->elementCount = 0; \
    } \
    inline void NAME##ArrayAppendConstArray(NAME ## Array *array, TYPE *other, size_t count) { \
        for(size_t i = 0; i < count; i++) { \
            NAME ## ArrayAddElement(array, other[i]); \
        } \
    } \
    inline void NAME##ArrayAddElement(NAME ## Array *array, TYPE element) { \
        if(array->elementCount + 1 > array->capacity) { \
            array->capacity *= 2; \
            array->elements = (TYPE*)realloc(array->elements, (array->capacity) * sizeof(TYPE)); \
            assert(array->elements != NULL); \
        } \
        array->elements[array->elementCount++] = element; \
    }

#endif
