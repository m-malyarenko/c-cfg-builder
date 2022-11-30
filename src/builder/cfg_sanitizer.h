#ifndef __CFG_CFG_SANITIZER_H__
#define __CFG_CFG_SANITIZER_H__

#include <stdbool.h>

#include "../entity/basic_block.h"
#include "../utility/basic_block_vec.h"

struct basic_block_vec* eliminate_empty_basic_blocks(const struct basic_block_vec* cfg_nodes);

struct basic_block_vec* merge_linear_basic_blocks(struct basic_block* entry, struct basic_block* sink);

bool merge_linear_basic_blocks_inner(struct basic_block* node,
                                     struct basic_block_vec* visited,
                                     struct basic_block_vec* accumulator,
                                     struct basic_block* sink);

#endif /* __CFG_CFG_SANITIZER_H__ */

