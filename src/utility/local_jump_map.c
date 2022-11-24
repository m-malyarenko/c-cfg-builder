#include <stdlib.h>

#include "local_jump_map.h"

struct local_jump_map* local_jump_map_new() {
    struct local_jump_map* new_lj_map = malloc(sizeof(struct local_jump_map));

    new_lj_map->cap = UTILITY_LOCAL_JUMP_MAP_DEFAULT_CAP;
    new_lj_map->len = 0;
    new_lj_map->buffer =
        malloc(new_lj_map->cap * sizeof(struct local_jump_map_node));

    return new_lj_map;
}

void local_jump_map_drop(struct local_jump_map** self) {
    if ((self == NULL) || (*self == NULL)) {
        return;
    }

    if ((*self)->buffer != NULL) {
        free((*self)->buffer);
    }

    free(*self);
    *self = NULL;
}

void local_jump_map_push(struct local_jump_map* self,
                         enum local_jump_type type,
                         struct basic_block** from,
                         struct control_flow* parent_cf)
{
    if ((self == NULL) || (from == NULL) || (parent_cf == NULL)) {
        return;
    }

    struct local_jump_map_node lj_map_node = {
        .type = type,
        .from = from,
        .parent_cf = parent_cf,
    };

    if (self->len == self->cap) {
        self->cap *= 2;
        self->buffer =
            realloc(self->buffer, self->cap * sizeof(struct local_jump_map_node));
    }

    self->buffer[self->len] = lj_map_node;
    self->len += 1;
}