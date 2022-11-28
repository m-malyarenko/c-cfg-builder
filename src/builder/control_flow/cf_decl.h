#ifndef __BUILDER_CONTROL_FLOW_CF_DECL_H__
#define __BUILDER_CONTROL_FLOW_CF_DECL_H__

#include <str.h>

#include "cf_expr.h"
#include "cf_node.h"

struct cf_decl {
    str_t* type_name;
    struct cf_node* expr;
};

struct cf_node* cf_decl_new(struct cf_node* parent);

void cf_decl_drop(struct cf_decl** self);

#endif /* __BUILDER_CONTROL_FLOW_CF_DECL_H__ */
