#include <stdlib.h>

#include "operation.h"
#include "basic_block.h"

struct basic_block* basic_block_new() {
    struct basic_block* new_basic_block = malloc(sizeof(struct basic_block));

    new_basic_block->tmp_count = 0;

    new_basic_block->links.cap = BB_LINKS_DEFAULT_CAP;
    new_basic_block->links.len = 0;
    new_basic_block->links.buffer =
        malloc(BB_LINKS_DEFAULT_CAP * sizeof(struct basic_block*));

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
        free((*self)->links.buffer);
    }

    if ((*self)->operations.buffer != NULL) {
        for (size_t i = 0; i < (*self)->operations.len; i++) {
            operation_drop(&(*self)->operations.buffer[i]);
        }

        free((*self)->operations.buffer);
    }

    free(*self);
    *self = NULL;
}

bool basic_block_is_empty(const struct basic_block* self) {
    return (self == NULL) ? true : (self->operations.len == 0);
}

void basic_block_push_operation(struct basic_block* self, struct operation* op) {
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

void basic_block_push_link(struct basic_block* self, struct basic_block* bb) {
    if ((self == NULL) || (bb == NULL)) {
        return;
    }

    if (self->links.len == self->links.cap) {
        self->links.cap *= 2;
        self->links.buffer =
            realloc(self->links.buffer, self->links.cap * sizeof(struct basic_block*));
    }

    self->links.buffer[self->operations.len] = bb;
    self->links.len += 1;
}

void basic_block_insert_link(struct basic_block* self, struct basic_block* bb, size_t idx) {
    if (self == NULL) {
        return;
    }

    if (idx >= self->links.len) {
        basic_block_resize_links(self, idx + 1);
    }

    self->links.buffer[idx] = bb;
}

void basic_block_resize_links(struct basic_block* self, size_t new_size) {
    if ((self == NULL) || (self->links.len == new_size)) {
        return;
    }

    if (self->links.len > new_size) {
        for (size_t i = new_size; i < self->links.len; i++) {
            self->links.buffer[i] = NULL;
        }
    } else {
        if (self->links.cap < new_size) {
            self->links.cap = new_size;
            self->links.buffer =
                realloc(self->links.buffer, self->links.cap * sizeof(struct basic_block*));
        }

        for (size_t i = self->links.len ; i < new_size; i++) {
            self->links.buffer[i] = NULL;
        }
    }

    self->links.len = new_size;
}

struct basic_block* basic_block_split_end(struct basic_block* self) {
    if (self == NULL) {
        return NULL;
    }

    if (self->operations.len == 0) {
        return self;
    }

    struct basic_block* new_basic_block = basic_block_new();
    basic_block_resize_links(new_basic_block, self->links.len);

    for (size_t i = 0; i < self->links.len; i++) {
        new_basic_block->links.buffer[i] = self->links.buffer[i];
    }

    basic_block_resize_links(self, 1);
    basic_block_insert_link(self, new_basic_block, 0);

    return new_basic_block; 
}

unsigned short basic_block_get_tmp_count(const struct basic_block* self) {
    return (self == NULL) ? 0 : self->tmp_count;
}

void basic_block_increment_tmp_count(struct basic_block* self) {
    if (self == NULL) {
        return;
    }

    self->tmp_count += 1;
}
