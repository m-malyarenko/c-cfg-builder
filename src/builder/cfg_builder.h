#ifndef __CFG_CFG_BUILDER_H__
#define __CFG_CFG_BUILDER_H__

#include <stdbool.h>

#include <clang-c/Index.h>
#include <ustring/str_list.h>

#include "../entity/basic_block.h"
#include "../utility/basic_block_vector.h"
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
    struct control_flow* current_control_flow;
    struct basic_block* current_basic_block;
    struct basic_block_vector* cfg_nodes;
    enum cfg_builder_status status;
};

struct cfg_builder_output build_function_cfg(CXCursor function_cursor);

VISITOR(visit_function);

VISITOR(visit_expression);

VISITOR(visit_statement);

VISITOR(visit_declaration);

VISITOR(visit_if_statement);

VISITOR(visit_while_statement);

VISITOR(visit_do_while_statement);

VISITOR(visit_for_statement);

VISITOR(visit_switch_statement);

VISITOR(inspect_children);

/* Utility visitors */

void push_spelling_to_basic_block(CXCursor cursor, struct basic_block* bb);

#endif /* __CFG_CFG_BUILDER_H__ */
