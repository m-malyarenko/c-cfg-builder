#ifndef __UTILITY_CHILDREN_VECTOR_H__
#define __UTILITY_CHILDREN_VECTOR_H__

#include <stddef.h>

#include <clang-c/Index.h>

#define UTILITY_CURSOR_VEC_DEFAULT_CAP ((size_t) 3)

struct cursor_vec {
    CXCursor* buffer;
    size_t len;
    size_t cap;
};

struct cursor_vec* cursor_vec_new();

void cursor_vec_drop(struct cursor_vec** self);

void cursor_vec_push(struct cursor_vec* self, CXCursor child);

CXCursor cursor_vec_pop(struct cursor_vec* self);

#endif /* __UTILITY_CHILDREN_VECTOR_H__ */
