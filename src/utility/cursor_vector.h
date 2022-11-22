#ifndef __UTILITY_CHILDREN_VECTOR_H__
#define __UTILITY_CHILDREN_VECTOR_H__

#include <stddef.h>

#include <clang-c/Index.h>

#define UTILITY_CHILDREN_VECTOR_DEVAULT_CAP ((size_t) 3)

struct cursor_vector {
    CXCursor* buffer;
    size_t len;
    size_t cap;
};

struct cursor_vector* cursor_vector_new();

void cursor_vector_drop(struct cursor_vector** self);

void cursor_vector_push(struct cursor_vector* self, CXCursor child);

CXCursor cursor_vector_pop(struct cursor_vector* self);

#endif /* __UTILITY_CHILDREN_VECTOR_H__ */
