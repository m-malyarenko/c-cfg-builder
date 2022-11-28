#ifndef __UTILITY_GOTO_TABLE_H__
#define __UTILITY_GOTO_TABLE_H__

#include <stddef.h>

#include "../builder/control_flow/cf_node.h"
#include "cf_node_vec.h"

enum goto_entry_type {
    GOTO_ENTRY_SRC,
    GOTO_ENTRY_DEST,
};

struct goto_table {
    struct cf_node_vec* src;
    struct cf_node_vec* dest;
};

struct goto_table* goto_table_new();

void goto_table_drop(struct goto_table** self);

void goto_table_push(struct goto_table* self, struct cf_node* entry, enum goto_entry_type type);

#endif /* __UTILITY_GOTO_TABLE_H__ */
