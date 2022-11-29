#include <stdlib.h>
#include <assert.h>

#include "cf_stmnt.h"
#include "cf_stmnt.h"
#include "cf_node.h"

struct cf_node* cf_stmnt_new(enum cf_stmnt_type type, void* impl, struct cf_node* parent) {
    struct cf_stmnt* new_cf_stmnt = malloc(sizeof(struct cf_stmnt));

    new_cf_stmnt->type = type;
    new_cf_stmnt->_impl = impl;

    struct cf_node* new_cf_node = cf_node_new(CF_NODE_STMNT, new_cf_stmnt, parent);

    return new_cf_node;
}

void cf_stmnt_drop(struct cf_stmnt** self) {
    if ((self == NULL) || (*self == NULL)) {
        return;
    }

    if ((*self)->_impl == NULL) {
        free(*self);
        *self = NULL;
        return;
    }

    switch ((*self)->type) {
        case CF_STMNT_COMPOUND: {
            cf_stmnt_compound_drop(&(*self)->_compound);
        } break;
        case CF_STMNT_IF: {
            cf_stmnt_if_drop(&(*self)->_if);
        } break;
        case CF_STMNT_SWITCH: {
            cf_stmnt_switch_drop(&(*self)->_switch);
        } break;
        case CF_STMNT_WHILE: {
            cf_stmnt_while_drop(&(*self)->_while);
        } break;
        case CF_STMNT_DO: {
            cf_stmnt_do_drop(&(*self)->_do);
        } break;
        case CF_STMNT_BREAK: break;
        case CF_STMNT_CONTINUE: break;
        case CF_STMNT_RETURN: {
            cf_stmnt_return_drop(&(*self)->_return);
        } break;
        case CF_STMNT_LABEL: {
            cf_stmnt_label_drop(&(*self)->_label);
        } break;
        case CF_STMNT_CASE: {
            cf_stmnt_case_drop(&(*self)->_case);
        } break;
        default:
            assert(0 && "Undefined statement type");
    }
}