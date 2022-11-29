#ifndef __BUILDER_CONTROL_FLOW_CF_STMNT_PATTERN_H__
#define __BUILDER_CONTROL_FLOW_CF_STMNT_PATTERN_H__

#include <str.h>

#include "../../utility/cf_node_vec.h"
#include "../../entity/basic_block.h"
#include "cf_node.h"

/* Compound statement */
struct cf_stmnt_compound {
    struct cf_node_vec* items;
};

struct cf_node* cf_stmnt_compound_new(struct cf_node* parent);

void cf_stmnt_compound_drop(struct cf_stmnt_compound** self);

/* Declaration statement */
struct cf_stmnt_decl {
    struct cf_node_vec* items;
};

struct cf_node* cf_stmnt_decl_new(struct cf_node* parent);

void cf_stmnt_decl_drop(struct cf_stmnt_decl** self);

/* If statement */
struct cf_stmnt_if {
    struct cf_node* cond;
    struct cf_node* body_true;
    struct cf_node* body_false;
};

struct cf_node* cf_stmnt_if_new(struct cf_node* parent);

void cf_stmnt_if_drop(struct cf_stmnt_if** self);

/* Switch statement */
struct cf_stmnt_switch {
    struct cf_node* select;
    struct cf_node* body;
};

struct cf_node* cf_stmnt_switch_new(struct cf_node* parent);

void cf_stmnt_switch_drop(struct cf_stmnt_switch** self);

/* While statement */
struct cf_stmnt_while {
    struct cf_node* cond;
    struct cf_node* body;
};

struct cf_node* cf_stmnt_while_new(struct cf_node* parent);

void cf_stmnt_while_drop(struct cf_stmnt_while** self);

/* Do statement */
struct cf_stmnt_do {
    struct cf_node* body;
    struct cf_node* cond;
};

struct cf_node* cf_stmnt_do_new(struct cf_node* parent);

void cf_stmnt_do_drop(struct cf_stmnt_do** self);

/* For statement */
struct cf_stmnt_for {
    struct cf_node* init;
    struct cf_node* cond;
    struct cf_node* body;
    struct cf_node* iter;
};

struct cf_node* cf_stmnt_for_new(struct cf_node* parent);

void cf_stmnt_for_drop(struct cf_stmnt_for** self);

/* Return statement */
struct cf_stmnt_return {
    struct cf_node* return_expr;
};

struct cf_node* cf_stmnt_return_new(struct cf_node* parent);

void cf_stmnt_return_drop(struct cf_stmnt_return** self);

/* Case statement */
struct cf_stmnt_case {
    struct cf_node* const_expr;
    struct cf_node* body;
    struct basic_block* imag_bb;
};

struct cf_node* cf_stmnt_case_new(struct cf_node* parent);

void cf_stmnt_case_drop(struct cf_stmnt_case** self);

/* Goto statement */
struct cf_stmnt_goto {
    str_t* label;
    struct basic_block* imag_bb;
};

struct cf_node* cf_stmnt_goto_new(struct cf_node* parent);

void cf_stmnt_goto_drop(struct cf_stmnt_goto** self);

/* Label statement */
struct cf_stmnt_label {
    str_t* label;
};

struct cf_node* cf_stmnt_label_new(struct cf_node* parent);

void cf_stmnt_label_drop(struct cf_stmnt_label** self);

#endif /* __BUILDER_CONTROL_FLOW_CF_STMNT_PATTERN_H__ */