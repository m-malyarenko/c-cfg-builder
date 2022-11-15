#ifndef __BUILDER_CONTROL_FLOW_CONTROL_FLOW_H__
#define __BUILDER_CONTROL_FLOW_CONTROL_FLOW_H__

#include <stddef.h>

#include "../../entity/basic_block.h"
#include "control_flow_type.h"

#define CF_TRUE_BRANCH_LINK_IDX ((size_t) 0)
#define CF_FALSE_BRANCH_LINK_IDX ((size_t) 1)

#define CF_SWITCH_DEFAULT_BRANCHES_NUM ((size_t) 3)

struct control_flow;

struct control_flow_if {
    struct basic_block* cond;
    struct control_flow* body;
};

struct control_flow_if_else {
    struct basic_block* cond;
    struct control_flow* body_true;
    struct control_flow* body_false;
};

struct control_flow_while {
    struct basic_block* cond;
    struct control_flow* body;
};

struct control_flow_do_while {
    struct control_flow* body;
    struct basic_block* cond;
};

struct control_flow_for {
    struct basic_block* init;
    struct basic_block* cond;
    struct control_flow* body;
    struct basic_block* iter;
};

struct control_flow_switch {
    struct basic_block* cond;
    struct {
        struct control_flow** branch_bodies;
        size_t len;
        size_t cap;
    } branches;
};

struct control_flow {
    enum control_flow_type type;
    struct basic_block* entry;
    union {
        void* cf_linear;
        struct control_flow_if* cf_if;
        struct control_flow_if_else* cf_if_else;
        struct control_flow_while* cf_while;
        struct control_flow_do_while* cf_do_while;
        struct control_flow_for* cf_for;
        struct control_flow_switch* cf_switch;
    } pattern;
    struct basic_block* exit;
};

struct control_flow* control_flow_new(enum control_flow_type type,
                                      struct basic_block* entry,
                                      struct basic_block* exit);

struct control_flow_if* control_flow_create_pattern_if(const struct control_flow* self);

struct control_flow_if_else* control_flow_create_pattern_if_else(const struct control_flow* self);

struct control_flow_while* control_flow_create_pattern_while(const struct control_flow* self);

struct control_flow_do_while* control_flow_create_pattern_do_while(const struct control_flow* self);

struct control_flow_for* control_flow_create_pattern_for(const struct control_flow* self);

struct control_flow_switch* control_flow_create_pattern_switch(const struct control_flow* self);

#endif /* __BUILDER_CONTROL_FLOW_CONTROL_FLOW_H__ */