#ifndef __BUILDER_CONTROL_FLOW_CF_NODE_H__
#define __BUILDER_CONTROL_FLOW_CF_NODE_H__

#include "../../entity/basic_block.h"
#include "cf_func.h"
#include "cf_stmnt.h"
#include "cf_decl.h"
#include "cf_expr.h"

enum cf_node_type {
    CF_NODE_NULL,
    CF_NODE_FUNC,
    CF_NODE_STMNT,
    CF_NODE_DECL,
    CF_NODE_EXPR,
};

struct cf_node {
    enum cf_node_type type;
    struct cf_node* parent;
    struct basic_block* entry;
    union {
        void* _impl;
        struct cf_func* _func;
        struct cf_stmnt* _stmnt;
        struct cf_decl* _decl;
        struct cf_expr* _expr;
    } node;
};

struct cf_node* cf_node_new(enum cf_node_type type, void* impl, struct cf_node* parent);

void cf_node_drop(struct cf_node** self);

#endif /* __BUILDER_CONTROL_FLOW_CF_NODE_H__ */
