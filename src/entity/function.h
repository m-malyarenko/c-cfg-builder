#ifndef __CFG_FUNCTION_H__
#define __CFG_FUNCTION_H__

#include "operation.h"
#include "basic_block.h"

struct function {
    struct basic_block* source;
    struct basic_block* sink;
};

#endif /* __CFG_FUNCTION_H__ */
