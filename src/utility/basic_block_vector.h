#ifndef __UTILITY_BASIC_BLOCK_VECTOR_H__
#define __UTILITY_BASIC_BLOCK_VECTOR_H__

#include <stddef.h>

#include "../entity/basic_block.h"

#define UTILITY_BASIC_BLOCK_VECTOR_DEVAULT_CAP ((size_t)3)

struct basic_block_vector
{
    struct basic_block **buffer;
    size_t len;
    size_t cap;
};

struct basic_block_vector *basic_block_vector_new();

void basic_block_vector_drop(struct basic_block_vector **self);

void basic_block_vector_push(struct basic_block_vector *self, struct basic_block *basic_block);

struct basic_block *basic_block_vector_pop(struct basic_block_vector *self);

#endif /* __UTILITY_BASIC_BLOCK_VECTOR_H__ */
