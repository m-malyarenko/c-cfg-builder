#ifndef __ENTITY_BASIC_BLOCK_H__
#define __ENTITY_BASIC_BLOCK_H__

#include <stddef.h>
#include <stdbool.h>

#include <ustring/str_list.h>

#define BB_OPERATIONS_DEFAULT_CAP ((size_t) 1)

typedef str_list_t operation_t;

struct basic_block {
    size_t ref_count;

    struct {
        operation_t** buffer;
        size_t len;
        size_t cap;
    } operations;

    struct {
        struct basic_block** buffer;
        size_t len;
    } links;
};

struct basic_block* basic_block_new();

void basic_block_drop(struct basic_block** self);

bool basic_block_is_empty(const struct basic_block* self);

void basic_block_push_operation(struct basic_block* self, operation_t* op);

const operation_t* basic_block_get_operation(const struct basic_block* self, size_t idx);

void basic_block_resize_links(struct basic_block* self, size_t new_size);

void basic_block_set_link(struct basic_block* self, struct basic_block* bb, size_t idx);

void basic_block_append_operations(struct basic_block* self, struct basic_block* donor);

#endif /* __ENTITY_BASIC_BLOCK_H__ */