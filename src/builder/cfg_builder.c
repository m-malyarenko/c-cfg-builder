#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include <clang-c/Index.h>

#include "../utility/cursor_vector.h"
#include "../utility/basic_block_vector.h"
#include "../utility/spelling.h"
#include "../entity/basic_block.h"
#include "../entity/operation.h"
#include "cfg_builder.h"

struct cfg_builder_output build_function_cfg(CXCursor function_cursor) {
    if (clang_Cursor_isNull(function_cursor)
            || clang_isInvalid(clang_getCursorKind(function_cursor)))
    {
        return (struct cfg_builder_output) { NULL, NULL };
    }

    /* Root control flow with dummy basic block as terminator */
    struct control_flow* root_control_flow = control_flow_new(CONTROL_FLOW_EMPTY, NULL);
    root_control_flow->pattern.cf_empty->ancestor_link = NULL;
    struct control_flow* sink_control_flow =
        control_flow_create_successor(root_control_flow, CONTROL_FLOW_LINEAR);

    struct basic_block* cfg_sink = sink_control_flow->entry;
    struct basic_block_vector* cfg_nodes = basic_block_vector_new();

    struct cfg_builder_handler handler = {
        .current_control_flow = root_control_flow,
        .current_basic_block = NULL,
        .cfg_nodes = cfg_nodes,
        .status = CFG_BUILDER_OK,
    };

    clang_visitChildren(function_cursor, visit_function, &handler);

    if (handler.status != CFG_BUILDER_OK) {
        // TODO Drop CFG
        basic_block_vector_drop(&cfg_nodes);
        return (struct cfg_builder_output) { NULL, NULL };
    }

    struct cfg_builder_output output = {
        .cfg_nodes = cfg_nodes,
        .cfg_sink = cfg_sink,
    };

    return output;
}

VISITOR(visit_function) {
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    const enum CXCursorKind parent_kind = clang_getCursorKind(parent);
    struct cfg_builder_handler* handler = client_data;

    if (cursor_kind == CXCursor_ParmDecl) {
        return CXVisit_Continue;
    }

    // TODO Если не было разрыва в последовательности expression, то продолжить в том же линейном control flow
    if (clang_isDeclaration(cursor_kind)) {
        visit_declaration(cursor, parent,handler);
    } else if (clang_isExpression(cursor_kind)) {
        visit_expression(cursor, parent, handler);
    } else if (clang_isStatement(cursor_kind)) {
        visit_statement(cursor, parent, handler);
    } else {
        handler->status = CFG_BUILDER_UNSUPPORTED_SYNTAX;
        return CXChildVisit_Break;
    }

    return CXChildVisit_Continue;
}

VISITOR(visit_expression) {
    // TODO Implement function
    return CXChildVisit_Continue;
}

VISITOR(visit_statement) {
    // TODO Implement function
    return CXChildVisit_Continue;
}

VISITOR(visit_declaration) {
    // TODO Implement function
    return CXChildVisit_Continue;
}

VISITOR(visit_if_statement) {
    // TODO Implement function
    return CXChildVisit_Continue;
}

VISITOR(visit_while_statement) {
    // TODO Implement function
    return CXChildVisit_Continue;
}

VISITOR(visit_do_while_statement) {
    // TODO Implement function
    return CXChildVisit_Continue;
}

VISITOR(visit_for_statement) {
    // TODO Implement function
    return CXChildVisit_Continue;
}

VISITOR(visit_switch_statement) {
    // TODO Implement function
    return CXChildVisit_Continue;
}

VISITOR(inspect_children) {
    struct cursor_vector* children = client_data;
    cursor_vector_push(children, cursor);

    return CXChildVisit_Continue;
}

void push_spelling_to_basic_block(CXCursor cursor, struct basic_block* bb) {
    str_t* expression_spelling = get_source_range_spelling(cursor);
    struct operation* operation = operation_new();
    operation_push_operand(operation, expression_spelling);
    basic_block_push_operation(bb, operation);
}