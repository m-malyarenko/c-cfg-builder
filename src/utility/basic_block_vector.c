#include <stdlib.h>

#include "basic_block_vector.h"

struct basic_block_vector* basic_block_vector_new() {
    struct basic_block_vector* new_basic_block_vector =
        malloc(sizeof(struct basic_block_vector));

    new_basic_block_vector->cap = UTILITY_BASIC_BLOCK_VECTOR_DEVAULT_CAP;
    new_basic_block_vector->buffer =
        malloc(new_basic_block_vector->cap * sizeof(struct basic_block*));
    new_basic_block_vector->len = 0;

    return new_basic_block_vector;
}

void basic_block_vector_drop(struct basic_block_vector** self) {
    if ((self == NULL) || (*self == NULL)) {
        return;
    }

    if ((*self)->buffer != NULL) {
        free((*self)->buffer);
    }
    
    free(*self);
    *self = NULL;
}

void basic_block_vector_push(struct basic_block_vector* self, struct basic_block* basic_block) {
    if (self == NULL) {
        return;
    }

    if (self->len == self->cap) {
        self->cap *= 2;
        self->buffer = realloc(self->buffer, self->cap * sizeof(struct basic_block*));
    }

    self->buffer[self->len] = basic_block;
    self->len += 1;
}

struct basic_block* basic_block_vector_pop(struct basic_block_vector* self) {
    if ((self == NULL) || (self->len == 0)) {
        return NULL;
    }

    struct basic_block* out_basic_block = self->buffer[self->len - 1];
    self->buffer[self->len - 1] = NULL;
    self->len -= 1;

    return out_basic_block;
}
