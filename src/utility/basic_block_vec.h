#ifndef __UTILITY_BASIC_BLOCK_VEC_H__
#define __UTILITY_BASIC_BLOCK_VEC_H__

#include <stddef.h>

#include "../entity/basic_block.h"

#define UTILITY_BASIC_BLOCK_VEC_DEFAULT_CAP ((size_t)3)

struct basic_block_vec
{
    struct basic_block **buffer;
    size_t len;
    size_t cap;
};

struct basic_block_vec *basic_block_vec_new();

void basic_block_vec_drop(struct basic_block_vec **self);

void basic_block_vec_push(struct basic_block_vec *self, struct basic_block *basic_block);

struct basic_block *basic_block_vec_pop(struct basic_block_vec *self);

#endif /* __UTILITY_BASIC_BLOCK_VEC_H__ */
