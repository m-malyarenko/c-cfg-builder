#include <stdlib.h>

#include <clang-c/Index.h>

#include "cursor_vector.h"

struct cursor_vector* cursor_vector_new() {
    struct cursor_vector* new_children_vector =
        malloc(sizeof(struct cursor_vector));

    new_children_vector->cap = UTILITY_CHILDREN_VECTOR_DEVAULT_CAP;
    new_children_vector->buffer =
        malloc(new_children_vector->cap * sizeof(CXCursor));
    new_children_vector->len = 0;

    return new_children_vector;
}

void cursor_vector_drop(struct cursor_vector** self) {
    if ((self == NULL) || (*self == NULL)) {
        return;
    }

    if ((*self)->buffer != NULL) {
        free((*self)->buffer);
    }
    
    free(*self);
    *self = NULL;
}

void cursor_vector_push(struct cursor_vector* self, CXCursor child) {
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

CXCursor cursor_vector_pop(struct cursor_vector* self) {
    if ((self == NULL) || (self->len == 0)) {
        return clang_getNullCursor();
    }

    CXCursor out_cursor = self->buffer[self->len - 1];
    self->buffer[self->len - 1] = clang_getNullCursor();
    self->len -= 1;

    return out_cursor;
}
