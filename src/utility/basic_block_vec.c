#include <stdlib.h>

#include "basic_block_vec.h"

struct basic_block_vec* basic_block_vec_new() {
    struct basic_block_vec* new_basic_block_vector =
        malloc(sizeof(struct basic_block_vec));

    new_basic_block_vector->cap = UTILITY_BASIC_BLOCK_VEC_DEFAULT_CAP;
    new_basic_block_vector->buffer =
        malloc(new_basic_block_vector->cap * sizeof(struct basic_block*));
    new_basic_block_vector->len = 0;

    return new_basic_block_vector;
}

void basic_block_vec_drop(struct basic_block_vec** self) {
    if ((self == NULL) || (*self == NULL)) {
        return;
    }

    if ((*self)->buffer != NULL) {
        free((*self)->buffer);
    }
    
    free(*self);
    *self = NULL;
}

void basic_block_vec_push(struct basic_block_vec* self, struct basic_block* basic_block) {
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

struct basic_block* basic_block_vec_pop(struct basic_block_vec* self) {
    if ((self == NULL) || (self->len == 0)) {
        return NULL;
    }

    struct basic_block* out_basic_block = self->buffer[self->len - 1];
    self->buffer[self->len - 1] = NULL;
    self->len -= 1;

    return out_basic_block;
}
