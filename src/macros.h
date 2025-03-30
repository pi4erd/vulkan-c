#ifndef MACROS_H_
#define MACROS_H_

#define ASSERT_ERR(R, BLOCK, ...) if(R != VK_SUCCESS) { \
        BLOCK \
        fprintf(stderr, __VA_ARGS__); \
        return R; \
    }

#endif
