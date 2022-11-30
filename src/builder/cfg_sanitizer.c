#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "../entity/basic_block.h"
#include "../utility/basic_block_vec.h"
#include "cfg_sanitizer.h"

struct basic_block_vec* eliminate_empty_basic_blocks(const struct basic_block_vec* cfg_nodes) {
    if ((cfg_nodes == NULL) || (cfg_nodes->len == 0)) {
        return NULL;
    }

    const size_t initial_nodes_num = cfg_nodes->len;
    struct basic_block_vec* sanitized_cfg_nodes = basic_block_vec_new();

    for (size_t i = 0; i < initial_nodes_num; i++) {
        struct basic_block* bb = cfg_nodes->buffer[i];

        if (bb->operations.len == 0) {
            continue;
        }

        for (size_t j = 0; j < bb->links.len; j++) {
            struct basic_block* linked_bb = bb->links.buffer[j];

            if ((linked_bb == NULL) || (linked_bb->links.buffer == NULL)) {
                continue;
            }

            if (linked_bb->operations.len == 0) {
                assert(linked_bb->ref_count == 1);

                basic_block_set_link(bb, linked_bb->links.buffer[0], j);
            }
        }

        basic_block_vec_push(sanitized_cfg_nodes, bb);
    }

    for (size_t i = 0; i < initial_nodes_num; i++) {
        struct basic_block* bb = cfg_nodes->buffer[i];

        if (bb->operations.len == 0) {
            basic_block_drop(&bb);
        }
    }

    return sanitized_cfg_nodes;
}

struct basic_block_vec* merge_linear_basic_blocks(struct basic_block* entry, struct basic_block* sink) {
    struct basic_block_vec* sanitized_cfg_nodes = basic_block_vec_new();
    struct basic_block_vec* visited_nodes = basic_block_vec_new();

    merge_linear_basic_blocks_inner(
        entry,
        visited_nodes,
        sanitized_cfg_nodes,
        sink
    );

    basic_block_vec_drop(&visited_nodes);

    return sanitized_cfg_nodes;
}

bool merge_linear_basic_blocks_inner(struct basic_block* node,
                                     struct basic_block_vec* visited,
                                     struct basic_block_vec* accumulator,
                                     struct basic_block* sink)
{   
    assert(node != NULL);
    assert(visited != NULL);
    assert(accumulator != NULL);

    for (size_t i = 0; i < visited->len; i++) {
        if (visited->buffer[i] == node) {
            return false;
        }
    }

    basic_block_vec_push(visited, node);

    if (node == sink) {
        basic_block_vec_push(accumulator, node);
        return false;    
    }

    const bool has_signle_link = (node->links.len == 1);

    for (size_t i = 0; i < node->links.len; i++) {
        struct basic_block* linked_bb = node->links.buffer[i];
    
        const bool can_merge_next =
            merge_linear_basic_blocks_inner(
                linked_bb,
                visited,
                accumulator,
                sink
            );

        if (!can_merge_next) {
            continue;
        }

        if (has_signle_link) {
            struct basic_block* next_bb = node->links.buffer[0];
            basic_block_append_operations(node, next_bb);
            basic_block_resize_links(node, next_bb->links.len);

            for (size_t j = 0; j < next_bb->links.len; j++) {
                basic_block_set_link(node, next_bb->links.buffer[j], j);
            }

            basic_block_drop(&next_bb);
        } else {
            basic_block_vec_push(accumulator, linked_bb);
        }
    }

    if (node->ref_count == 1) {
        return true;
    } else {
        basic_block_vec_push(accumulator, node);
        return false;
    }
}
