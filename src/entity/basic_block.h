#ifndef __CFG_BASIC_BLOCK_H__
#define __CFG_BASIC_BLOCK_H__

#include <stddef.h>

#include "operation.h"

#define BASIC_BLOCK_LINKS_DEFAULT_CAP ((size_t) 2)

struct basic_block {
    struct {
        struct operation** buffer;
        size_t len;
        size_t cap;
    } operations;

    struct {
        struct basic_block** buffer;
        size_t len;
        size_t cap;
    } links;
};

#endif /* __CFG_BASIC_BLOCK_H__ */