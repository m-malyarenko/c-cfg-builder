#include <stdlib.h>
#include <assert.h>

#include "cf_node.h"

struct cf_node* cf_node_new(enum cf_node_type type, void* impl, struct cf_node* parent) {
    struct cf_node* new_cf_node = malloc(sizeof(struct cf_node));

    new_cf_node->type = type;
    new_cf_node->node._impl = (type == CF_NODE_NULL) ? NULL : impl;
    new_cf_node->parent = parent;
    new_cf_node->entry = NULL;

    return new_cf_node;
}

void cf_node_drop(struct cf_node** self) {
    if ((self == NULL) || (*self == NULL)) {
        return;
    }

    switch ((*self)->type) {
        case CF_NODE_FUNC: {
            cf_func_drop(&(*self)->node._func);
        } break;
        case CF_NODE_STMNT: {
            cf_stmnt_drop(&(*self)->node._stmnt);
        } break;
        case CF_NODE_DECL: {
            cf_decl_drop(&(*self)->node._decl);
        } break;
        case CF_NODE_EXPR: {
            cf_expr_drop(&(*self)->node._expr);
        } break;
        default:
        case CF_NODE_NULL: {
            assert((*self)->node._impl == NULL);
        } break;
    }

    free(*self);
    *self = NULL;
}
