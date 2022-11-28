#include <stdlib.h>

#include <str.h>

#include "cf_node.h"
#include "cf_decl.h"

struct cf_node* cf_decl_new(struct cf_node* parent) {
    struct cf_decl* new_cf_decl = malloc(sizeof(struct cf_decl));

    new_cf_decl->type_name = str_new(NULL);
    new_cf_decl->expr = NULL;

    return cf_node_new(CF_NODE_DECL, new_cf_decl, parent);
}

void cf_decl_drop(struct cf_decl** self) {
    if ((self == NULL) || (*self == NULL)) {
        return;
    }

    str_drop(&(*self)->type_name);
    cf_node_drop(&(*self)->expr);

    free(*self);
    *self = NULL;
}
