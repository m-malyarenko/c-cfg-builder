#ifndef __CFG_CFG_BUILDER_H__
#define __CFG_CFG_BUILDER_H__

#include <stdbool.h>

#include <clang-c/Index.h>
#include <ustring/str_list.h>

#include "../entity/basic_block.h"
#include "../utility/basic_block_vector.h"
#include "../utility/local_jump_map.h"
#include "control_flow/control_flow.h"

#define VISITOR(name) enum CXChildVisitResult name(CXCursor cursor, CXCursor parent, CXClientData client_data)

enum cfg_builder_status {
    CFG_BUILDER_OK,
    CFG_BUILDER_UNSUPPORTED_SYNTAX,
};

struct cfg_builder_output {
    struct basic_block_vector* cfg_nodes;
    struct basic_block* cfg_sink;
};

struct cfg_builder_handler {
    struct control_flow* current_cf;
    struct basic_block* current_bb;
    struct basic_block** current_cf_ancestor_link[2];
    struct control_flow* current_lj_cf;
    struct basic_block_vector* cfg_nodes;
    struct local_jump_map* lj_map;
    struct basic_block* sink_bb;
    enum cfg_builder_status status;
};

/* Entry point */

struct cfg_builder_output build_function_cfg(CXCursor function_cursor);

/* Visitors */

VISITOR(visit_function);

VISITOR(visit_expression);

VISITOR(visit_declaration);

VISITOR(visit_statement);

VISITOR(visit_compound_statement);

VISITOR(visit_if_statement);

VISITOR(visit_while_statement);

VISITOR(visit_do_while_statement);

VISITOR(visit_for_statement);

VISITOR(visit_switch_statement);

VISITOR(visit_return_statement);

VISITOR(visit_break_statement);

VISITOR(visit_continue_statement);

VISITOR(visit_goto_statement);

VISITOR(inspect_children);

/* Utility */

struct cursor_vector* get_cursor_children(CXCursor cursor);

bool can_put_expression_in_current_bb(const struct cfg_builder_handler* handler);

void push_spelling_to_basic_block(CXCursor cursor, struct basic_block* bb);

#endif /* __CFG_CFG_BUILDER_H__ */
