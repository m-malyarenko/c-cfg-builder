#ifndef __UTILITY_CF_NODE_VEC_H__
#define __UTILITY_CF_NODE_VEC_H__

#include <stddef.h>

#include "../builder/control_flow/cf_node.h"

#define UTILITY_CF_NODE_DEFAULT_CAP ((size_t) 2)

struct cf_node_vec {
    struct cf_node** buffer;
    size_t len;
    size_t cap;
};

struct cf_node_vec* cf_node_vec_new();

void cf_node_vec_drop(struct cf_node_vec** self);

void cf_node_vec_push(struct cf_node_vec* self, struct cf_node* node);

#endif /* __UTILITY_CF_NODE_VEC_H__ */
