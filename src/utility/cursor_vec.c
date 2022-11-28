#include <stdlib.h>

#include <clang-c/Index.h>

#include "cursor_vec.h"

struct cursor_vec* cursor_vec_new() {
    struct cursor_vec* new_cursor_vec = malloc(sizeof(struct cursor_vec));

    new_cursor_vec->cap = UTILITY_CURSOR_VEC_DEFAULT_CAP;
    new_cursor_vec->buffer =
        malloc(new_cursor_vec->cap * sizeof(CXCursor));
    new_cursor_vec->len = 0;

    return new_cursor_vec;
}

void cursor_vec_drop(struct cursor_vec** self) {
    if ((self == NULL) || (*self == NULL)) {
        return;
    }

    if ((*self)->buffer != NULL) {
        free((*self)->buffer);
    }
    
    free(*self);
    *self = NULL;
}

void cursor_vec_push(struct cursor_vec* self, CXCursor child) {
    if (self == NULL) {
        return;
    }

    if (self->len == self->cap) {
        self->cap *= 2;
        self->buffer = realloc(self->buffer, self->cap * sizeof(CXCursor));
    }

    self->buffer[self->len] = child;
    self->len += 1;
}

CXCursor cursor_vec_pop(struct cursor_vec* self) {
    if ((self == NULL) || (self->len == 0)) {
        return clang_getNullCursor();
    }

    CXCursor out_cursor = self->buffer[self->len - 1];
    self->buffer[self->len - 1] = clang_getNullCursor();
    self->len -= 1;

    return out_cursor;
}
