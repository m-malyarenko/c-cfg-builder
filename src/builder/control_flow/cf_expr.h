#ifndef __BUILDER_CONTROL_FLOW_CF_EXPR_H__
#define __BUILDER_CONTROL_FLOW_CF_EXPR_H__

#include "../../entity/basic_block.h"
#include "cf_node.h"

struct cf_expr {
    struct basic_block* bb;
};

struct cf_node* cf_expr_new(struct cf_node* parent);

void cf_expr_drop(struct cf_expr** self);

#endif /* __BUILDER_CONTROL_FLOW_CF_EXPR_H__ */
