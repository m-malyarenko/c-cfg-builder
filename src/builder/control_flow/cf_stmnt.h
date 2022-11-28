#ifndef __BUILDER_CONTROL_FLOW_CF_STMNT_H__
#define __BUILDER_CONTROL_FLOW_CF_STMNT_H__

#include <stddef.h>

#include "cf_stmnt_pattern.h"
#include "cf_node.h"

enum cf_stmnt_type {
    CF_STMNT_COMPOUND,
    CF_STMNT_DECL,
    CF_STMNT_IF,
    CF_STMNT_SWITCH,
    CF_STMNT_WHILE,
    CF_STMNT_DO,
    CF_STMNT_FOR,
    CF_STMNT_BREAK,
    CF_STMNT_CONTINUE,
    CF_STMNT_RETURN,
    CF_STMNT_GOTO,
    CF_STMNT_LABEL,
    CF_STMNT_CASE,
    CF_STMNT_DEFAULT,
};

struct cf_stmnt {
    enum cf_stmnt_type type;
    union {
        void* _impl;
        struct cf_stmnt_compound* _compound;
        struct cf_stmnt_decl* _decl;
        struct cf_stmnt_if* _if;
        struct cf_stmnt_switch* _switch;
        struct cf_stmnt_while* _while;
        struct cf_stmnt_do* _do;
        struct cf_stmnt_for* _for;
        struct cf_stmnt_return* _return;
        struct cf_stmnt_goto* _goto;
        struct cf_stmnt_label* _label;
        struct cf_stmnt_case* _case;
        struct cf_stmnt_default* _default;
    };
};

struct cf_node* cf_stmnt_new(enum cf_stmnt_type type, void* impl, struct cf_node* parent);

void cf_stmnt_drop(struct cf_stmnt** self);

#endif /* __BUILDER_CONTROL_FLOW_CF_STMNT_H__ */
