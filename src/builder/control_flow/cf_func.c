#include <stdlib.h>

#include "cf_stmnt_pattern.h"
#include "cf_stmnt.h"
#include "cf_node.h"
#include "cf_func.h"

struct cf_node* cf_func_new() {
    struct cf_func* new_cf_func = malloc(sizeof(struct cf_func));
    struct cf_node* new_cf_node = cf_node_new(CF_NODE_FUNC, new_cf_func, NULL);

    new_cf_func->body = NULL;

    return new_cf_node;
}

void cf_func_drop(struct cf_func** self) {
    if ((self == NULL) || (*self == NULL)) {
        return;
    }

    cf_node_drop(&(*self)->body);

    free(*self);
    *self = NULL;
}

void cf_func_push_node(struct cf_func* self, struct cf_node* node) {
    if ((self == NULL) || (node == NULL)) {
        return;
    }

    cf_node_vec_push(self->body->node._stmnt->_compound->items, node);
}
