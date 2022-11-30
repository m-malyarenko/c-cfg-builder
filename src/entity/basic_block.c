#include <stdlib.h>
#include <assert.h>

#include "basic_block.h"

struct basic_block* basic_block_new() {
    struct basic_block* new_basic_block = malloc(sizeof(struct basic_block));

    new_basic_block->ref_count = 0;

    new_basic_block->links.len = 0;
    new_basic_block->links.buffer = NULL;

    new_basic_block->operations.cap = BB_OPERATIONS_DEFAULT_CAP;
    new_basic_block->operations.len = 0;
    new_basic_block->operations.buffer =
        malloc(BB_OPERATIONS_DEFAULT_CAP * sizeof(struct operation*));

    return new_basic_block;
}

void basic_block_drop(struct basic_block** self) {
    if ((self == NULL) || (*self == NULL)) {
        return;
    }

    if ((*self)->links.buffer != NULL) {
        for (size_t i = 0; i < (*self)->links.len; i++) {
            (*self)->links.buffer[i]->ref_count -= 1;
        }

        free((*self)->links.buffer);
    }

    if ((*self)->operations.buffer != NULL) {
        for (size_t i = 0; i < (*self)->operations.len; i++) {
            str_list_drop(&(*self)->operations.buffer[i]);
        }

        free((*self)->operations.buffer);
    }

    free(*self);
    *self = NULL;
}

bool basic_block_is_empty(const struct basic_block* self) {
    return (self == NULL) ? true : (self->operations.len == 0);
}

void basic_block_push_operation(struct basic_block* self, operation_t* op) {
    if ((self == NULL) || (op == NULL)) {
        return;
    }

    if (self->operations.len == self->operations.cap) {
        self->operations.cap *= 2;
        self->operations.buffer =
            realloc(self->operations.buffer, self->operations.cap * sizeof(struct operation*));
    }

    self->operations.buffer[self->operations.len] = op;
    self->operations.len += 1;
}

const operation_t* basic_block_get_operation(const struct basic_block* self, size_t idx) {
    if (self == NULL) {
        return NULL;
    }

    assert(idx < self->operations.len);

    return self->operations.buffer[idx];
}

void basic_block_resize_links(struct basic_block* self, size_t new_size) {
    if ((self == NULL) || (self->links.len == new_size)) {
        return;
    }

    self->links.buffer =
        realloc(self->links.buffer, self->links.len * sizeof(struct basic_block*));
    
    for (size_t i = self->links.len; i < new_size; i++) {
        self->links.buffer[i] = NULL;
    }

    self->links.len = new_size;
}

void basic_block_set_link(struct basic_block* self, struct basic_block* bb, size_t idx) {
    if (self == NULL) {
        return;
    }

    assert(idx < self->links.len);

    if (self->links.buffer[idx] != NULL) {
        self->links.buffer[idx]->ref_count -= 1;
    }

    self->links.buffer[idx] = bb;

    if (bb != NULL) {
        bb->ref_count += 1;
    }
}

void basic_block_append_operations(struct basic_block* self, struct basic_block* donor) {
    if ((self == NULL) || (donor == NULL)) {
        return;
    }

    const size_t donor_op_num = donor->operations.len;
    for (size_t i = 0; i < donor_op_num; i++) {
        operation_t* op_copy = str_list_copy(basic_block_get_operation(donor, i));
        basic_block_push_operation(self, op_copy);
    }
}