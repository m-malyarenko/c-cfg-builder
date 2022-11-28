#include <stdlib.h>

#include "../../entity/basic_block.h"
#include "cf_node.h"
#include "cf_expr.h"

struct cf_node* cf_expr_new(struct cf_node* parent) {
    struct cf_expr* new_cf_expr = malloc(sizeof(struct cf_node));

    new_cf_expr->bb = basic_block_new();

    return cf_node_new(CF_NODE_EXPR, new_cf_expr, parent);
}

void cf_expr_drop(struct cf_expr** self) {
    if ((self == NULL) || (*self == NULL)) {
        return;
    }

    basic_block_drop(&(*self)->bb);

    free(*self);
    *self = NULL;
}