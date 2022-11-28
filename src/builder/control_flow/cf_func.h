#ifndef __BUILDER_CONTROL_FLOW_CF_FUNC_H__
#define __BUILDER_CONTROL_FLOW_CF_FUNC_H__

#include "cf_node.h"

struct cf_func {
    struct cf_node* body;
};

struct cf_node* cf_func_new();

void cf_func_drop(struct cf_func** self);

void cf_func_push_node(struct cf_func* self, struct cf_node* node);

#endif /* __BUILDER_CONTROL_FLOW_CF_FUNC_H__ */
