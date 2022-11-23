#ifndef __ENTITY_BASIC_BLOCK_H__
#define __ENTITY_BASIC_BLOCK_H__

#include <stddef.h>
#include <stdbool.h>

#include "operation.h"

#define BB_OPERATIONS_DEFAULT_CAP ((size_t) 1)

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

struct basic_block* basic_block_new();

void basic_block_drop(struct basic_block** self);

bool basic_block_is_empty(const struct basic_block* self);

void basic_block_push_operation(struct basic_block* self, struct operation* op);

void basic_block_resize_links(struct basic_block* self, size_t new_size);

void basic_block_set_link(struct basic_block* self, struct basic_block* bb, size_t idx);

struct basic_block* basic_block_split_end(struct basic_block* self);

#endif /* __ENTITY_BASIC_BLOCK_H__ */