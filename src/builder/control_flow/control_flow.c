#include <stdlib.h>

#include "../../entity/basic_block.h"
#include "control_flow_type.h"
#include "control_flow.h"

struct control_flow* control_flow_new(enum control_flow_type type,
                                      struct basic_block* entry,
                                      struct basic_block* exit)
{
    struct control_flow* cf_new = malloc(sizeof(struct control_flow));

    cf_new->entry = (entry == NULL) ? basic_block_new() : entry;
    cf_new->exit = exit;
    cf_new->type = type;

    switch (type) {
        case CONTROL_FLOW_IF:
            cf_new->pattern.cf_if = control_flow_create_pattern_if(cf_new);
        case CONTROL_FLOW_IF_ELSE:
            cf_new->pattern.cf_if_else = control_flow_create_pattern_if_else(cf_new);
            break;
        case CONTROL_FLOW_WHILE:
            cf_new->pattern.cf_while = control_flow_create_pattern_while(cf_new);
            break;
        case CONTROL_FLOW_DO_WHILE:
            cf_new->pattern.cf_do_while = control_flow_create_pattern_do_while(cf_new);
            break;
        case CONTROL_FLOW_FOR:
            cf_new->pattern.cf_for = control_flow_create_pattern_for(cf_new);
            break;
        case CONTROL_FLOW_SWITCH:
            cf_new->pattern.cf_switch = control_flow_create_pattern_switch(cf_new);
            break;
        default:
        case CONTROL_FLOW_LINEAR: {
            cf_new->pattern.cf_linear = NULL;
        } break;
    }
}

struct control_flow_if* control_flow_create_pattern_if(const struct control_flow* self) {
    if (self == NULL) {
        return NULL;
    }

    struct control_flow_if* pattern = malloc(sizeof(struct control_flow_if));

    pattern->cond = self->entry;
    pattern->body = control_flow_new(CONTROL_FLOW_LINEAR, NULL, self->exit);

    basic_block_resize_links(pattern->cond, 2);
    basic_block_insert_link(pattern->cond, pattern->body->entry, CF_TRUE_BRANCH_LINK_IDX);
    basic_block_insert_link(pattern->cond, self->exit, CF_FALSE_BRANCH_LINK_IDX);

    return pattern;
}

struct control_flow_if_else* control_flow_create_pattern_if_else(const struct control_flow* self) {
    if (self == NULL) {
        return NULL;
    }

    struct control_flow_if_else* pattern = malloc(sizeof(struct control_flow_if_else));

    pattern->cond = self->entry;
    pattern->body_true = control_flow_new(CONTROL_FLOW_LINEAR, NULL, self->exit);
    pattern->body_false = control_flow_new(CONTROL_FLOW_LINEAR, NULL, self->exit);

    basic_block_resize_links(pattern->cond, 2);
    basic_block_insert_link(pattern->cond, pattern->body_true->entry, CF_TRUE_BRANCH_LINK_IDX);
    basic_block_insert_link(pattern->cond, pattern->body_false->entry, CF_FALSE_BRANCH_LINK_IDX);

    return pattern;
}

struct control_flow_while* control_flow_create_pattern_while(const struct control_flow* self) {
    if (self == NULL) {
        return NULL;
    }

    struct control_flow_while* pattern = malloc(sizeof(struct control_flow_while));

    pattern->cond = self->entry;
    pattern->body = control_flow_new(CONTROL_FLOW_LINEAR, pattern->cond, pattern->cond);

    basic_block_resize_links(pattern->cond, 2);
    basic_block_insert_link(pattern->cond, pattern->body->entry, CF_TRUE_BRANCH_LINK_IDX);
    basic_block_insert_link(pattern->cond, self->exit, CF_FALSE_BRANCH_LINK_IDX);

    return pattern;
}

struct control_flow_do_while* control_flow_create_pattern_do_while(const struct control_flow* self) {
    if (self == NULL) {
        return NULL;
    }

    struct control_flow_do_while* pattern = malloc(sizeof(struct control_flow_do_while));

    pattern->cond = basic_block_new();
    pattern->body = control_flow_new(CONTROL_FLOW_LINEAR, self->entry, pattern->cond);

    basic_block_resize_links(pattern->cond, 2);
    basic_block_insert_link(pattern->cond, pattern->body->entry, CF_TRUE_BRANCH_LINK_IDX);
    basic_block_insert_link(pattern->cond, self->exit, CF_FALSE_BRANCH_LINK_IDX);

    return pattern;
}

struct control_flow_for* control_flow_create_pattern_for(const struct control_flow* self) {
    if (self == NULL) {
        return NULL;
    }

    struct control_flow_for* pattern = malloc(sizeof(struct control_flow_for));

    pattern->init = self->entry;
    pattern->cond = basic_block_new();
    pattern->iter = basic_block_new();
    pattern->body = control_flow_new(CONTROL_FLOW_LINEAR, NULL, pattern->iter);

    /* Init block */
    basic_block_resize_links(pattern->init, 1);
    basic_block_insert_link(pattern->init, pattern->cond, 0);

    /* Condition block */
    basic_block_resize_links(pattern->cond, 2);
    basic_block_insert_link(pattern->cond, pattern->body->entry, CF_TRUE_BRANCH_LINK_IDX);
    basic_block_insert_link(pattern->cond, self->exit, CF_FALSE_BRANCH_LINK_IDX);

    /* Iteration block */
    basic_block_resize_links(pattern->iter, 1);
    basic_block_insert_link(pattern->iter, pattern->cond, 0);

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
