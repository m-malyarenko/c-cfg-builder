#include <stdlib.h>
#include <assert.h>

#include "cf_stmnt.h"
#include "cf_node.h"
#include "cf_stmnt_pattern.h"

struct cf_node* cf_stmnt_compound_new(struct cf_node* parent) {
    struct cf_stmnt_compound* new_cf_stmnt_compound = malloc(sizeof(struct cf_stmnt_compound));

    new_cf_stmnt_compound->items = cf_node_vec_new();

    return cf_stmnt_new(CF_STMNT_COMPOUND, new_cf_stmnt_compound, parent);
}

void cf_stmnt_compound_drop(struct cf_stmnt_compound** self) {
    if ((self == NULL) || (*self == NULL)) {
        return;
    }

    cf_node_vec_drop(&(*self)->items);

    free(*self);
    *self = NULL;
}

struct cf_node* cf_stmnt_decl_new(struct cf_node* parent) {
    struct cf_stmnt_decl* new_cf_stmnt_decl = malloc(sizeof(struct cf_stmnt_decl));

    new_cf_stmnt_decl->items = cf_node_vec_new();

    return cf_stmnt_new(CF_STMNT_DECL, new_cf_stmnt_decl, parent);
}

void cf_stmnt_decl_drop(struct cf_stmnt_decl** self) {
    if ((self == NULL) || (*self == NULL)) {
        return;
    }

    cf_node_vec_drop(&(*self)->items);

    free(*self);
    *self = NULL;
}

struct cf_node* cf_stmnt_if_new(struct cf_node* parent) {
    struct cf_stmnt_if* new_cf_stmnt_if = malloc(sizeof(struct cf_stmnt_if));

    new_cf_stmnt_if->cond = NULL;
    new_cf_stmnt_if->body_true = NULL;
    new_cf_stmnt_if->body_false = NULL;

    return cf_stmnt_new(CF_STMNT_IF, new_cf_stmnt_if, parent);
}

void cf_stmnt_if_drop(struct cf_stmnt_if** self) {
    if ((self == NULL) || (*self == NULL)) {
        return;
    }

    cf_node_drop(&(*self)->cond);
    cf_node_drop(&(*self)->body_true);
    cf_node_drop(&(*self)->body_false);

    free(*self);
    *self = NULL;
}

struct cf_node* cf_stmnt_switch_new(struct cf_node* parent) {
    struct cf_stmnt_switch* new_cf_stmnt_switch = malloc(sizeof(struct cf_stmnt_switch));

    new_cf_stmnt_switch->select = NULL;
    new_cf_stmnt_switch->body = NULL;

    return cf_stmnt_new(CF_STMNT_SWITCH, new_cf_stmnt_switch, parent);
}

void cf_stmnt_switch_drop(struct cf_stmnt_switch** self) {
    if ((self == NULL) || (*self == NULL)) {
        return;
    }

    cf_node_drop(&(*self)->select);
    cf_node_drop(&(*self)->body);

    free(*self);
    *self = NULL;
}

struct cf_node* cf_stmnt_while_new(struct cf_node* parent) {
    struct cf_stmnt_while* new_cf_stmnt_while = malloc(sizeof(struct cf_stmnt_while));

    new_cf_stmnt_while->cond = NULL;
    new_cf_stmnt_while->body = NULL;

    return cf_stmnt_new(CF_STMNT_WHILE, new_cf_stmnt_while, parent);
}

void cf_stmnt_while_drop(struct cf_stmnt_while** self) {
    if ((self == NULL) || (*self == NULL)) {
        return;
    }

    cf_node_drop(&(*self)->cond);
    cf_node_drop(&(*self)->body);

    free(*self);
    *self = NULL;
}

struct cf_node* cf_stmnt_do_new(struct cf_node* parent) {
    struct cf_stmnt_do* new_cf_stmnt_do = malloc(sizeof(struct cf_stmnt_do));

    new_cf_stmnt_do->body = NULL;
    new_cf_stmnt_do->cond = NULL;

    return cf_stmnt_new(CF_STMNT_DO, new_cf_stmnt_do, parent);
}

void cf_stmnt_do_drop(struct cf_stmnt_do** self) {
    if ((self == NULL) || (*self == NULL)) {
        return;
    }

    cf_node_drop(&(*self)->body);
    cf_node_drop(&(*self)->cond);

    free(*self);
    *self = NULL;
}

struct cf_node* cf_stmnt_for_new(struct cf_node* parent) {
    struct cf_stmnt_for* new_cf_stmnt_for = malloc(sizeof(struct cf_stmnt_for));

    new_cf_stmnt_for->init = NULL;
    new_cf_stmnt_for->cond = NULL;
    new_cf_stmnt_for->body = NULL;
    new_cf_stmnt_for->iter = NULL;

    return cf_stmnt_new(CF_STMNT_FOR, new_cf_stmnt_for, parent);
}

void cf_stmnt_for_drop(struct cf_stmnt_for** self) {
    if ((self == NULL) || (*self == NULL)) {
        return;
    }

    cf_node_drop(&(*self)->init);
    cf_node_drop(&(*self)->cond);
    cf_node_drop(&(*self)->body);
    cf_node_drop(&(*self)->iter);

    free(*self);
    *self = NULL;
}

struct cf_node* cf_stmnt_return_new(struct cf_node* parent) {
    struct cf_stmnt_return* new_cf_stmnt_return = malloc(sizeof(struct cf_stmnt_return));

    new_cf_stmnt_return->return_expr = NULL;

    return cf_stmnt_new(CF_STMNT_RETURN, new_cf_stmnt_return, parent);
}

void cf_stmnt_return_drop(struct cf_stmnt_return** self) {
    if ((self == NULL) || (*self == NULL)) {
        return;
    }

    cf_node_drop(&(*self)->return_expr);

    free(*self);
    *self = NULL;
}

struct cf_node* cf_stmnt_case_new(struct cf_node* parent) {
    struct cf_stmnt_case* new_cf_stmnt_case = malloc(sizeof(struct cf_stmnt_case));

    new_cf_stmnt_case->const_expr = NULL;
    new_cf_stmnt_case->body = NULL;
    new_cf_stmnt_case->imag_bb = basic_block_new();

    return cf_stmnt_new(CF_STMNT_CASE, new_cf_stmnt_case, parent);
}

void cf_stmnt_case_drop(struct cf_stmnt_case** self) {
    if ((self == NULL) || (*self == NULL)) {
        return;
    }

    cf_node_drop(&(*self)->const_expr);
    cf_node_drop(&(*self)->body);

    free(*self);
    *self = NULL;
}

struct cf_node* cf_stmnt_goto_new(struct cf_node* parent) {
    struct cf_stmnt_goto* new_cf_stmnt_goto = malloc(sizeof(struct cf_stmnt_goto));

    new_cf_stmnt_goto->label = str_new(NULL);
    new_cf_stmnt_goto->imag_bb = basic_block_new();

    return cf_stmnt_new(CF_STMNT_GOTO, new_cf_stmnt_goto, parent);
}

void cf_stmnt_goto_drop(struct cf_stmnt_goto** self) {
    if ((self == NULL) || (*self == NULL)) {
        return;
    }

    str_drop(&(*self)->label);

    free(*self);
    *self = NULL;
}

struct cf_node* cf_stmnt_label_new(struct cf_node* parent) {
    struct cf_stmnt_label* new_cf_stmnt_label = malloc(sizeof(struct cf_stmnt_label));

    new_cf_stmnt_label->label = str_new(NULL);

    return cf_stmnt_new(CF_STMNT_LABEL, new_cf_stmnt_label, parent);
}

void cf_stmnt_label_drop(struct cf_stmnt_label** self) {
    if ((self == NULL) || (*self == NULL)) {
        return;
    }

    str_drop(&(*self)->label);

    free(*self);
    *self = NULL;
}
