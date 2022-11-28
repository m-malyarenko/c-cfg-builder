#include <stdlib.h>

#include "cf_node_vec.h"

struct cf_node_vec* cf_node_vec_new() {
    struct cf_node_vec* new_cf_node_vec = malloc(sizeof(struct cf_node_vec));

    new_cf_node_vec->cap = UTILITY_CF_NODE_DEFAULT_CAP;
    new_cf_node_vec->buffer = malloc(new_cf_node_vec->cap * sizeof(struct cf_node*));
    new_cf_node_vec->len = 0;

    return new_cf_node_vec;
}

void cf_node_vec_drop(struct cf_node_vec** self) {
    if ((self == NULL) || (*self == NULL)) {
        return;
    }

    if ((*self)->buffer != NULL) {
        free((*self)->buffer);
    }
    
    free(*self);
    *self = NULL;
}

void cf_node_vec_push(struct cf_node_vec* self, struct cf_node* node) {
    if (self == NULL) {
        return;
    }

    if (self->len == self->cap) {
        self->cap *= 2;
        self->buffer = realloc(self->buffer, self->cap * sizeof(struct cf_node*));
    }

    self->buffer[self->len] = node;
    self->len += 1;
}
