#ifndef __ENTITY_FUNCTION_H__
#define __ENTITY_FUNCTION_H__

#include "basic_block.h"

struct function {
    struct basic_block* source;
    struct basic_block* const sink;
};

#define ENTITY_FUNCTION_NULL() ((struct function) {NULL, NULL})

#define ENTITY_FUNCTION_IS_NULL(f) (((f).sink == NULL) && ((f).source == NULL))

#define ENTITY_FUNCTION_NEW() ((struct function) { .source = basic_block_new(), .sink = NULL })

#endif /* __ENTITY_FUNCTION_H__ */
