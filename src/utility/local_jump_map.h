#ifndef __UTILITY_LOCAL_JUMP_MAP_H__
#define __UTILITY_LOCAL_JUMP_MAP_H__

#include <stddef.h>

#include "../entity/basic_block.h"
#include "../builder/control_flow/control_flow.h"

#define UTILITY_LOCAL_JUMP_MAP_DEFAULT_CAP ((size_t) 3)

enum local_jump_type {
    LJ_BREAK,
    LJ_CONTINUE,
};

struct local_jump_map_node {
    enum local_jump_type type;
    struct basic_block** from;
    struct control_flow* parent_cf;
};

struct local_jump_map {
    struct local_jump_map_node* buffer;
    size_t cap;
    size_t len;
};

struct local_jump_map* local_jump_map_new();

void local_jump_map_drop(struct local_jump_map** self);

void local_jump_map_push(struct local_jump_map* self,
                         enum local_jump_type type,
                         struct basic_block** from,
                         struct control_flow* parent_cf);

#endif /* __UTILITY_LOCAL_JUMP_MAP_H__ */
