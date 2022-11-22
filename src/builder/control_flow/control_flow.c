#include <stdlib.h>
#include <assert.h>

#include "../../entity/basic_block.h"
#include "control_flow_type.h"
#include "control_flow.h"

struct control_flow* control_flow_new(enum control_flow_type type, struct control_flow* next)
{
    struct control_flow* cf_new = malloc(sizeof(struct control_flow));

    cf_new->type = type;
    cf_new->next = next;

    switch (type) {
        case CONTROL_FLOW_LINEAR: {
            cf_new->pattern.cf_linear = control_flow_create_pattern_linear(cf_new);
            cf_new->entry = cf_new->pattern.cf_linear->body;
        } break;
        case CONTROL_FLOW_IF: {
            cf_new->pattern.cf_if = control_flow_create_pattern_if(cf_new);
            cf_new->entry = cf_new->pattern.cf_if->cond;
        } break;
        case CONTROL_FLOW_IF_ELSE: {
            cf_new->pattern.cf_if_else = control_flow_create_pattern_if_else(cf_new);
            cf_new->entry = cf_new->pattern.cf_if_else->cond;
        } break;
        case CONTROL_FLOW_WHILE: {
            cf_new->pattern.cf_while = control_flow_create_pattern_while(cf_new);
            cf_new->entry = cf_new->pattern.cf_while->cond;
        } break;
        case CONTROL_FLOW_DO_WHILE: {
            cf_new->pattern.cf_do_while = control_flow_create_pattern_do_while(cf_new);
            cf_new->entry = cf_new->pattern.cf_do_while->body->entry;
        } break;
        case CONTROL_FLOW_FOR: {
            cf_new->pattern.cf_for = control_flow_create_pattern_for(cf_new);
            cf_new->entry = cf_new->pattern.cf_for->init;
        } break;
        case CONTROL_FLOW_SWITCH: {
            cf_new->pattern.cf_switch = control_flow_create_pattern_switch(cf_new);
            // TODO Switch control flow
        } break;
        default:
        case CONTROL_FLOW_EMPTY: {
            cf_new->pattern.cf_empty = control_flow_create_pattern_empty(cf_new);
            cf_new->entry = (next == NULL) ? NULL : next->entry; // NOTE: Ancestor link must be set by caller
        } break;
    }

    return cf_new;
}

void control_flow_drop(struct control_flow** self) {
    // TODO Implement
}

struct control_flow* control_flow_create_successor(struct control_flow* self, enum control_flow_type successor_type) {
    assert(self != NULL);

    if (self == NULL) {
        return NULL;
    }

    struct control_flow* cf_successor = control_flow_new(successor_type, self->next);
    control_flow_connect_tail_recursive(self, cf_successor->entry);

    cf_successor->next = self->next;
    self->next = cf_successor;

    return cf_successor; 
}

void control_flow_connect_tail_recursive(struct control_flow* self, struct basic_block* target) {
    assert(self != NULL);

    switch (self->type) {
        case CONTROL_FLOW_LINEAR: {
            basic_block_insert_link(self->pattern.cf_linear->body, target, 0);
        } break;
        case CONTROL_FLOW_IF: {
            basic_block_insert_link(self->pattern.cf_if->cond, target, CF_FALSE_BRANCH_LINK_IDX);

            struct control_flow* last_in_branch =
                control_flow_search_in_chain(self->pattern.cf_if->body, self->next);
            assert(last_in_branch != NULL);

            control_flow_connect_tail_recursive(last_in_branch, target);
        } break;
        case CONTROL_FLOW_IF_ELSE: {
            struct control_flow* last_in_true_branch =
                control_flow_search_in_chain(self->pattern.cf_if_else->body_true, self->next);
            assert(last_in_true_branch != NULL);

            struct control_flow* last_in_false_branch =
                control_flow_search_in_chain(self->pattern.cf_if_else->body_false, self->next);
            assert(last_in_false_branch != NULL);

            control_flow_connect_tail_recursive(last_in_true_branch, target);
            control_flow_connect_tail_recursive(last_in_false_branch, target);
        } break;
        case CONTROL_FLOW_WHILE: {
            basic_block_insert_link(self->pattern.cf_while->cond, target, CF_FALSE_BRANCH_LINK_IDX);
        } break;
        case CONTROL_FLOW_DO_WHILE: {
            basic_block_insert_link(self->pattern.cf_do_while->cond, target, CF_FALSE_BRANCH_LINK_IDX);
        } break;
        case CONTROL_FLOW_FOR: {
            basic_block_insert_link(self->pattern.cf_for->cond, target, CF_FALSE_BRANCH_LINK_IDX);
        } break;
        case CONTROL_FLOW_SWITCH: {
            for (size_t i = 0; i < self->pattern.cf_switch->branches.len; i++) {
                struct control_flow* branch_flow =
                    self->pattern.cf_switch->branches.branch_bodies[i];

                struct control_flow* last_in_branch =
                    control_flow_search_in_chain(branch_flow, self->next);
                assert(last_in_branch != NULL);

                control_flow_connect_tail_recursive(last_in_branch, target);
            }
        } break;
        default:
        case CONTROL_FLOW_EMPTY: {
            self->entry = target;
            if (self->pattern.cf_empty->ancestor_link != NULL) {
                *(self->pattern.cf_empty->ancestor_link) = self->entry;
            }
        } break;
    }
}

struct control_flow* control_flow_search_in_chain(struct control_flow* first, struct control_flow* target_next) {
    if (first == NULL) {
        return NULL;
    }

    struct control_flow* current_chain_link = first;

    while ((current_chain_link->next != NULL)
                && (current_chain_link->next != target_next))
    {
        current_chain_link = current_chain_link->next;
    }

    return (current_chain_link->next == NULL) ? NULL : current_chain_link;
}

struct control_flow_empty* control_flow_create_pattern_empty(const struct control_flow* self) {
    if (self == NULL) {
        return NULL;
    }

    struct control_flow_empty* pattern = malloc(sizeof(struct control_flow_empty));
    pattern->ancestor_link = NULL; // NOTE: Must be set by caller

    return pattern; 
}

struct control_flow_linear* control_flow_create_pattern_linear(const struct control_flow* self) {
    if (self == NULL) {
        return NULL;
    }

    struct control_flow_linear* pattern = malloc(sizeof(struct control_flow_linear));
    struct basic_block* next_entry = (self->next == NULL) ? NULL : self->next->entry;

    pattern->body = basic_block_new();
    basic_block_resize_links(pattern->body, 1);
    basic_block_insert_link(pattern->body, next_entry, 0);

    return pattern;
}

struct control_flow_if* control_flow_create_pattern_if(const struct control_flow* self) {
    if (self == NULL) {
        return NULL;
    }

    struct control_flow_if* pattern = malloc(sizeof(struct control_flow_if));
    struct basic_block* next_entry = (self->next == NULL) ? NULL : self->next->entry;

    pattern->cond = basic_block_new();
    basic_block_resize_links(pattern->cond, 2);
    basic_block_insert_link(pattern->cond, pattern->body->entry, CF_TRUE_BRANCH_LINK_IDX);
    basic_block_insert_link(pattern->cond, self->next->entry, CF_FALSE_BRANCH_LINK_IDX);
    pattern->body = control_flow_new(CONTROL_FLOW_EMPTY, self->next);

    pattern->body->pattern.cf_empty->ancestor_link =
        &pattern->cond->links.buffer[CF_TRUE_BRANCH_LINK_IDX];

    return pattern;
}

struct control_flow_if_else* control_flow_create_pattern_if_else(const struct control_flow* self) {
    if (self == NULL) {
        return NULL;
    }

    struct control_flow_if_else* pattern = malloc(sizeof(struct control_flow_if_else));

    pattern->cond = basic_block_new();
    pattern->body_true = control_flow_new(CONTROL_FLOW_EMPTY, self->next);
    pattern->body_false = control_flow_new(CONTROL_FLOW_EMPTY, self->next);

    basic_block_resize_links(pattern->cond, 2);
    basic_block_insert_link(pattern->cond, pattern->body_true->entry, CF_TRUE_BRANCH_LINK_IDX);
    basic_block_insert_link(pattern->cond, pattern->body_false->entry, CF_FALSE_BRANCH_LINK_IDX);

    pattern->body_true->pattern.cf_empty->ancestor_link =
        &pattern->cond->links.buffer[CF_TRUE_BRANCH_LINK_IDX];

    pattern->body_false->pattern.cf_empty->ancestor_link =
        &pattern->cond->links.buffer[CF_FALSE_BRANCH_LINK_IDX];

    return pattern;
}

struct control_flow_while* control_flow_create_pattern_while(const struct control_flow* self) {
    if (self == NULL) {
        return NULL;
    }

    struct control_flow_while* pattern = malloc(sizeof(struct control_flow_while));
    struct basic_block* next_entry = (self->next == NULL) ? NULL : self->next->entry;

    pattern->cond = basic_block_new();
    pattern->body = control_flow_new(CONTROL_FLOW_EMPTY, self->next);

    basic_block_resize_links(pattern->cond, 2);
    basic_block_insert_link(pattern->cond, pattern->body->entry, CF_TRUE_BRANCH_LINK_IDX);
    basic_block_insert_link(pattern->cond, next_entry, CF_FALSE_BRANCH_LINK_IDX);

    pattern->body->pattern.cf_empty->ancestor_link =
        &pattern->cond->links.buffer[CF_TRUE_BRANCH_LINK_IDX];

    return pattern;
}

struct control_flow_do_while* control_flow_create_pattern_do_while(const struct control_flow* self) {
    if (self == NULL) {
        return NULL;
    }

    struct control_flow_do_while* pattern = malloc(sizeof(struct control_flow_do_while));
    struct basic_block* next_entry = (self->next == NULL) ? NULL : self->next->entry;

    pattern->cond = basic_block_new();
    pattern->body = control_flow_new(CONTROL_FLOW_EMPTY, self->next);

    basic_block_resize_links(pattern->cond, 2);
    basic_block_insert_link(pattern->cond, pattern->body->entry, CF_TRUE_BRANCH_LINK_IDX);
    basic_block_insert_link(pattern->cond, next_entry, CF_FALSE_BRANCH_LINK_IDX);

    pattern->body->pattern.cf_empty->ancestor_link =
        &pattern->cond->links.buffer[CF_TRUE_BRANCH_LINK_IDX];

    return pattern;
}

struct control_flow_for* control_flow_create_pattern_for(const struct control_flow* self) {
    if (self == NULL) {
        return NULL;
    }

    struct control_flow_for* pattern = malloc(sizeof(struct control_flow_for));
    struct basic_block* next_entry = (self->next == NULL) ? NULL : self->next->entry;

    pattern->init = basic_block_new();
    pattern->cond = basic_block_new();
    pattern->iter = basic_block_new();

    /* Create special body control flow */
    pattern->body = malloc(sizeof(struct control_flow));

    pattern->body->type = CONTROL_FLOW_EMPTY;
    pattern->body->entry = pattern->iter;
    pattern->body->pattern.cf_empty = NULL;
    pattern->body->next = NULL;

    /* Init block */
    basic_block_resize_links(pattern->init, 1);
    basic_block_insert_link(pattern->init, pattern->cond, 0);

    /* Condition block */
    basic_block_resize_links(pattern->cond, 2);
    basic_block_insert_link(pattern->cond, pattern->body->entry, CF_TRUE_BRANCH_LINK_IDX);
    basic_block_insert_link(pattern->cond, next_entry, CF_FALSE_BRANCH_LINK_IDX);

    /* Iteration block */
    basic_block_resize_links(pattern->iter, 1);
    basic_block_insert_link(pattern->iter, pattern->cond, 0);

    pattern->body->pattern.cf_empty->ancestor_link =
        &pattern->cond->links.buffer[CF_TRUE_BRANCH_LINK_IDX];

    return pattern;
}

struct control_flow_switch* control_flow_create_pattern_switch(const struct control_flow* self) {
    if (self == NULL) {
        return NULL;
    }

    struct control_flow_switch* pattern = malloc(sizeof(struct control_flow_switch));

    pattern->cond = self->entry;
    pattern->branches.cap = CF_SWITCH_DEFAULT_BRANCHES_NUM;
    pattern->branches.branch_bodies = malloc(pattern->branches.cap * sizeof(struct control_flow*));
    pattern->branches.len = 0;

    // TODO Отдельно проработать добавление веток

    return pattern;
}
