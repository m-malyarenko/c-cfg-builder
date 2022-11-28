#include <stdlib.h>
#include <assert.h>

#include "../builder/control_flow/cf_node.h"
#include "cf_node_vec.h"
#include "goto_table.h"

struct goto_table* goto_table_new() {
    struct goto_table* new_goto_table = malloc(sizeof(struct goto_table));

    new_goto_table->src = cf_node_vec_new();
    new_goto_table->dest = cf_node_vec_new();

    return new_goto_table;
}

void goto_table_drop(struct goto_table** self) {
    if((self == NULL) || (*self == NULL)) {
        return;
    }

    cf_node_vec_drop(&(*self)->src);
    cf_node_vec_drop(&(*self)->dest);

    free(*self);
    *self = NULL;
}

void goto_table_push(struct goto_table* self, struct cf_node* entry, enum goto_entry_type type) {
    if ((self == NULL) || (entry == NULL)) {
        return;
    }

    switch (type) {
    case GOTO_ENTRY_SRC: {
        cf_node_vec_push(self->src, entry);
    } break;
    case GOTO_ENTRY_DEST: {
        cf_node_vec_push(self->dest, entry);
    } break;
    default: {
        assert(0 && "Invalid goto tableentry type");
    } break;
    }
}