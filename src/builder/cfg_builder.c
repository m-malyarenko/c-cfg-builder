#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include <clang-c/Index.h>

#include "../utility/cursor_vector.h"
#include "../utility/basic_block_vector.h"
#include "../utility/local_jump_map.h"
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
    struct control_flow* root_control_flow = control_flow_new(CF_EMPTY, NULL);
    root_control_flow->pattern.cf_empty->ancestor_link = NULL;
    struct control_flow* sink_control_flow =
        control_flow_create_successor(root_control_flow, CF_LINEAR);

    struct basic_block* cfg_sink = sink_control_flow->entry;
    struct basic_block_vector* cfg_nodes = basic_block_vector_new();

    struct cfg_builder_handler handler = {
        .current_cf = root_control_flow,
        .current_bb = NULL,
        .current_cf_ancestor_link = { NULL, NULL },
        .current_lj_cf = NULL,
        .cfg_nodes = cfg_nodes,
        .lj_map = local_jump_map_new(),
        .sink_bb = cfg_sink,
        .status = CFG_BUILDER_OK,
    };

    /* Function traversing entry */
    clang_visitChildren(function_cursor, visit_function, &handler);

    if (handler.status != CFG_BUILDER_OK) {
        // TODO Drop CFG
        basic_block_vector_drop(&cfg_nodes);
        return (struct cfg_builder_output) { NULL, NULL };
    }

    basic_block_vector_push(cfg_nodes, cfg_sink);

    struct cfg_builder_output output = {
        .cfg_nodes = cfg_nodes,
        .cfg_sink = cfg_sink,
    };

    return output;
}

VISITOR(visit_function) {
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    struct cfg_builder_handler* handler = client_data;

    if (cursor_kind == CXCursor_ParmDecl) {
        return CXVisit_Continue;
    }

    enum CXChildVisitResult result = CXChildVisit_Continue;

    if (clang_isDeclaration(cursor_kind)) {
        result = visit_declaration(cursor, parent,handler);
    } else if (clang_isExpression(cursor_kind)) {
        result = visit_expression(cursor, parent, handler);
    } else if (clang_isStatement(cursor_kind)) {
        result = visit_statement(cursor, parent, handler);
    } else {
        handler->status = CFG_BUILDER_UNSUPPORTED_SYNTAX;
        result =  CXChildVisit_Break;
    }

    return result;
}

VISITOR(visit_expression) {
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    struct cfg_builder_handler* handler = client_data;

    assert(clang_isExpression(cursor_kind));

    if (clang_isUnexposed(cursor_kind)) {
        clang_visitChildren(cursor, visit_expression, handler);
        return CXChildVisit_Continue;
    }

    const bool continue_in_basic_block = can_put_expression_in_current_bb(handler);

    if (!continue_in_basic_block) {
        struct control_flow* linear_successor =
            control_flow_create_successor(handler->current_cf, CF_LINEAR);
        handler->current_cf = linear_successor;
        handler->current_bb = handler->current_cf->entry;
        basic_block_vector_push(handler->cfg_nodes, handler->current_bb);
    }

    push_spelling_to_basic_block(cursor, handler->current_bb);

    return CXChildVisit_Continue;
}

VISITOR(visit_declaration) {
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    struct cfg_builder_handler* handler = client_data;

    assert(clang_isDeclaration(cursor_kind));

    enum CXChildVisitResult result = CXChildVisit_Continue;

    if (cursor_kind != CXCursor_VarDecl) {
        handler->status = CFG_BUILDER_UNSUPPORTED_SYNTAX;
        return CXChildVisit_Break;
    }

    const bool continue_in_basic_block = can_put_expression_in_current_bb(handler);

    if (!continue_in_basic_block) {
        struct control_flow* linear_successor =
            control_flow_create_successor(handler->current_cf, CF_LINEAR);
        handler->current_cf = linear_successor;
        handler->current_bb = handler->current_cf->entry;
        basic_block_vector_push(handler->cfg_nodes, handler->current_bb);
    }

    push_spelling_to_basic_block(cursor, handler->current_bb);

    return result;
}

VISITOR(visit_statement) {
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    struct cfg_builder_handler* handler = client_data;

    assert(clang_isStatement(cursor_kind));

    if (cursor_kind == CXCursor_NullStmt) {
        return CXChildVisit_Continue;
    }

    if (clang_isUnexposed(cursor_kind)) {
        const int visit_status =
            clang_visitChildren(cursor, visit_statement, handler);
        return (visit_status == 0) ? CXChildVisit_Continue : CXChildVisit_Break;
    }

    if (cursor_kind == CXCursor_CompoundStmt) {
        return visit_compound_statement(cursor, parent, handler);
    }

    enum CXChildVisitResult result = CXChildVisit_Continue;

    switch (cursor_kind) {
        case CXCursor_DeclStmt: {
            const int visit_status =
                clang_visitChildren(cursor, visit_declaration, handler);
            result = (visit_status == 0) ? CXChildVisit_Continue : CXChildVisit_Break;
        } break;
        case CXCursor_IfStmt: {
            result = visit_if_statement(cursor, parent, handler);
        } break;
        case CXCursor_WhileStmt: {
            result = visit_while_statement(cursor, parent, handler);
        } break;
        case CXCursor_DoStmt: {
            result = visit_do_while_statement(cursor, parent, handler);
        } break;
        case CXCursor_ForStmt: {
            result = visit_for_statement(cursor, parent, handler);
        } break;
        case CXCursor_SwitchStmt: {
            result = visit_switch_statement(cursor, parent, handler);
        } break;
        case CXCursor_ReturnStmt: {
            result = visit_return_statement(cursor, parent, handler);
        } break;
        case CXCursor_BreakStmt: {
            result = visit_break_statement(cursor, parent, handler);
        } break;
        case CXCursor_ContinueStmt: {
            result = visit_continue_statement(cursor, parent, handler);
        } break;
        case CXCursor_GotoStmt: {
            result = visit_goto_statement(cursor, parent, handler);
        } break;
        default: {
            handler->status = CFG_BUILDER_UNSUPPORTED_SYNTAX;
            result = CXChildVisit_Break;
        }
    }

    return result;
}

VISITOR(visit_compound_statement) {
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    struct cfg_builder_handler* handler = client_data;

    assert(cursor_kind == CXCursor_CompoundStmt);

    struct cursor_vector* children = get_cursor_children(cursor);

    /* Manualy visit all children */
    enum CXChildVisitResult result = CXChildVisit_Continue;
    for (size_t i = 0; i < children->len; i++) {
        CXCursor child_cursor = children->buffer[i];
        const enum CXCursorKind child_cursor_kind = clang_getCursorKind(child_cursor);

        if (clang_isDeclaration(child_cursor_kind)) {
            result = visit_declaration(child_cursor, cursor,handler);
        } else if (clang_isExpression(child_cursor_kind)) {
            result = visit_expression(child_cursor, cursor, handler);
        } else if (clang_isStatement(child_cursor_kind)) {
            result = visit_statement(child_cursor, cursor, handler);
        } else {
            handler->status = CFG_BUILDER_UNSUPPORTED_SYNTAX;
            result = CXChildVisit_Break;
        }

        if (result == CXChildVisit_Break) {
            break;
        }
    }

    cursor_vector_drop(&children);

    return result;
}

VISITOR(visit_if_statement) {
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    struct cfg_builder_handler* handler = client_data;

    assert(cursor_kind == CXCursor_IfStmt);

    struct cursor_vector* children = get_cursor_children(cursor);
    assert((children->len == 2) || (children->len == 3));

    const bool has_else_branch = (children->len == 3);
    const enum control_flow_type cf_type = has_else_branch ? CF_IF_ELSE : CF_IF;

    struct control_flow* const if_successor =
        control_flow_create_successor(handler->current_cf, cf_type);

    handler->current_cf = if_successor;
    handler->current_bb = if_successor->entry;
    basic_block_vector_push(handler->cfg_nodes, handler->current_bb);

    /* Condition */
    CXCursor condition_cursor = children->buffer[0];
    assert(clang_isExpression(clang_getCursorKind(condition_cursor)));
    visit_expression(condition_cursor, cursor, handler);

    /* Body */
    enum CXChildVisitResult result = CXChildVisit_Continue;

    for (size_t i = 1; i < children->len; i++) {
        CXCursor body_cursor = children->buffer[i];
        const enum CXCursorKind body_cursor_kind = clang_getCursorKind(body_cursor);

        if (has_else_branch) {               
            handler->current_cf =
                (i == 1)
                    ? if_successor->pattern.cf_if_else->body_true
                    : if_successor->pattern.cf_if_else->body_false;
        } else {
            handler->current_cf = if_successor->pattern.cf_if->body;
        }
        
        handler->current_bb = NULL; // Current control flow is empty

        if (clang_isExpression(body_cursor_kind)) {
            result = visit_expression(body_cursor, cursor, handler);
        } else if (clang_isStatement(body_cursor_kind)) {
            result = visit_statement(body_cursor, cursor, handler);
        } else {
            handler->status = CFG_BUILDER_UNSUPPORTED_SYNTAX;
            result = CXChildVisit_Break;
            break;
        }
    }

    /* Restore context */
    handler->current_cf = if_successor;
    handler->current_bb = NULL; // Control flow is over

    cursor_vector_drop(&children);

    return result;
}

VISITOR(visit_while_statement) {
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    struct cfg_builder_handler* handler = client_data;

    assert(cursor_kind == CXCursor_WhileStmt);

    struct cursor_vector* children = get_cursor_children(cursor);
    assert(children->len == 2); // Condition, Body

    /* Save context */
    struct control_flow* const saved_lj_cf = handler->current_lj_cf;

    struct control_flow* const while_successor =
        control_flow_create_successor(handler->current_cf, CF_WHILE);

    handler->current_cf = while_successor;
    handler->current_bb = while_successor->pattern.cf_while->cond;
    handler->current_lj_cf = while_successor;
    basic_block_vector_push(handler->cfg_nodes, handler->current_bb);

    /* Condition */
    CXCursor condition_cursor = children->buffer[0];
    assert(clang_isExpression(clang_getCursorKind(condition_cursor)));
    visit_expression(condition_cursor, cursor, handler);

    /* Body */
    CXCursor body_cursor = children->buffer[1];
    const enum CXCursorKind body_cursor_kind = clang_getCursorKind(body_cursor);
    enum CXChildVisitResult result = CXChildVisit_Continue;

    handler->current_cf = while_successor->pattern.cf_while->body;
    handler->current_bb = NULL; // Current control flow is empty

    if (clang_isExpression(body_cursor_kind)) {
        result = visit_expression(body_cursor, cursor, handler);
    } else if (clang_isStatement(body_cursor_kind)) {
        result = visit_statement(body_cursor, cursor, handler);
    } else {
        handler->status = CFG_BUILDER_UNSUPPORTED_SYNTAX;
        result = CXChildVisit_Break;
    }

    /* Restore context */
    handler->current_lj_cf = saved_lj_cf;
    handler->current_cf = while_successor;
    handler->current_bb = NULL; // Control flow is over

    cursor_vector_drop(&children);

    return result;
}

VISITOR(visit_do_while_statement) {
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    struct cfg_builder_handler* handler = client_data;

    assert(cursor_kind == CXCursor_DoStmt);

    struct cursor_vector* children = get_cursor_children(cursor);
    assert(children->len == 2); // Body, Condition

    /* Save context */
    struct control_flow* const saved_lj_cf = handler->current_lj_cf;

    struct control_flow* const do_successor =
        control_flow_create_successor(handler->current_cf, CF_DO_WHILE);

    handler->current_cf = do_successor;
    handler->current_bb = NULL;
    handler->current_lj_cf = do_successor;

    /* Body */
    CXCursor body_cursor = children->buffer[0];
    const enum CXCursorKind body_cursor_kind = clang_getCursorKind(body_cursor);
    enum CXChildVisitResult result = CXChildVisit_Continue;

    handler->current_cf = do_successor->pattern.cf_do_while->body;
    handler->current_bb = do_successor->pattern.cf_do_while->body->entry;
    basic_block_vector_push(handler->cfg_nodes, handler->current_bb);

    if (clang_isExpression(body_cursor_kind)) {
        result = visit_expression(body_cursor, cursor, handler);
    } else if (clang_isStatement(body_cursor_kind)) {
        result = visit_statement(body_cursor, cursor, handler);
    } else {
        handler->status = CFG_BUILDER_UNSUPPORTED_SYNTAX;
        handler->current_lj_cf = saved_lj_cf;
        handler->current_cf = do_successor;
        cursor_vector_drop(&children);
        return CXChildVisit_Break;
    }

    /* Condition */
    CXCursor condition_cursor = children->buffer[1];
    assert(clang_isExpression(clang_getCursorKind(condition_cursor)));

    handler->current_cf = do_successor->pattern.cf_do_while->cond;
    handler->current_bb = do_successor->pattern.cf_do_while->cond->entry;
    basic_block_vector_push(handler->cfg_nodes, handler->current_bb);
    visit_expression(condition_cursor, cursor, handler);

    /* Restore context */
    handler->current_lj_cf = saved_lj_cf;
    handler->current_cf = do_successor;
    handler->current_bb = NULL; // Control flow is over

    cursor_vector_drop(&children);

    return result;
}

VISITOR(visit_for_statement) {
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    struct cfg_builder_handler* handler = client_data;

    assert(cursor_kind == CXCursor_ForStmt);

    struct cursor_vector* children = get_cursor_children(cursor);

    if (children->len != 4) {
        handler->status = CFG_BUILDER_UNSUPPORTED_SYNTAX;
        cursor_vector_drop(&children);
        return CXChildVisit_Break;
    }

    /* Save context */
    struct control_flow* const saved_lj_cf = handler->current_lj_cf;

    struct control_flow* const for_successor =
        control_flow_create_successor(handler->current_cf, CF_FOR);

    handler->current_cf = for_successor;
    handler->current_lj_cf = for_successor;

    /* Init */
    CXCursor init_cursor = children->buffer[0];
    const enum CXCursorKind init_cursor_kind = clang_getCursorKind(init_cursor); 
    assert(clang_isExpression(init_cursor_kind) || (init_cursor_kind == CXCursor_DeclStmt));

    handler->current_bb = handler->current_cf->pattern.cf_for->init;
    basic_block_vector_push(handler->cfg_nodes, handler->current_bb);

    if (clang_isExpression(init_cursor_kind)) {
        visit_expression(init_cursor, cursor, handler);
    } else {
        clang_visitChildren(init_cursor, visit_declaration, handler);
    }
    
    /* Condition */
    CXCursor cond_cursor = children->buffer[1];
    assert(clang_isExpression(clang_getCursorKind(cond_cursor)));

    handler->current_bb = handler->current_cf->pattern.cf_for->cond;
    basic_block_vector_push(handler->cfg_nodes, handler->current_bb);
    visit_expression(cond_cursor, cursor, handler);

    /* Iter */
    CXCursor iter_cursor = children->buffer[2];
    assert(clang_isExpression(clang_getCursorKind(iter_cursor)));

    handler->current_cf = handler->current_cf->pattern.cf_for->iter;
    handler->current_bb = handler->current_cf->pattern.cf_linear->body;
    basic_block_vector_push(handler->cfg_nodes, handler->current_bb);
    visit_expression(iter_cursor, cursor, handler);

    /* Body */
    CXCursor body_cursor = children->buffer[3];
    const enum CXCursorKind body_cursor_kind = clang_getCursorKind(body_cursor);

    handler->current_cf = for_successor->pattern.cf_for->body;
    handler->current_bb = NULL; // Current control flow is empty

    enum CXChildVisitResult result = CXChildVisit_Continue;

    if (clang_isExpression(body_cursor_kind)) {
        result = visit_expression(body_cursor, cursor, handler);
    } else if (clang_isStatement(body_cursor_kind)) {
        result = visit_statement(body_cursor, cursor, handler);
    } else {
        handler->status = CFG_BUILDER_UNSUPPORTED_SYNTAX;
        result = CXChildVisit_Break;
    }

    /* Restore context */
    handler->current_lj_cf = saved_lj_cf;
    handler->current_cf = for_successor;
    handler->current_bb = NULL; // Control flow is over

    cursor_vector_drop(&children);

    return result;
}

VISITOR(visit_switch_statement) {
    // TODO Implement function
    return CXChildVisit_Continue;
}

VISITOR(visit_return_statement) {
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    struct cfg_builder_handler* handler = client_data;

    assert(cursor_kind == CXCursor_ReturnStmt);

    struct cursor_vector* children = get_cursor_children(cursor);

    if (children->len > 0) {
        CXCursor cond_cursor = children->buffer[0];
        struct control_flow* const saved_cf = handler->current_cf;
        assert(clang_isExpression(clang_getCursorKind(cond_cursor)));

        visit_expression(cond_cursor, cursor, handler);
        basic_block_set_link(handler->current_cf->entry, handler->sink_bb, 0);
    } else {
        control_flow_connect_tail_recursive(handler->current_cf, handler->sink_bb);
    }

    return CXChildVisit_Continue;
}

VISITOR(visit_break_statement) {
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    struct cfg_builder_handler* handler = client_data;

    assert(cursor_kind == CXCursor_BreakStmt);

    control_flow_connect_tail_recursive(handler->current_cf, handler->current_lj_cf->next->entry);

    struct control_flow* stub_successor =
        control_flow_create_successor(handler->current_cf, CF_EMPTY);

    handler->current_cf = stub_successor;
    handler->current_bb = NULL;

    return CXChildVisit_Continue;
}

VISITOR(visit_continue_statement) {
    // TODO Implement function
    return CXChildVisit_Continue;
}

VISITOR(visit_goto_statement) {
    // TODO Implement function
    return CXChildVisit_Continue;
}

VISITOR(inspect_children) {
    struct cursor_vector* children = client_data;
    cursor_vector_push(children, cursor);

    return CXChildVisit_Continue;
}

struct cursor_vector* get_cursor_children(CXCursor cursor) {
    struct cursor_vector* children = cursor_vector_new();
    clang_visitChildren(cursor, inspect_children, children);

    return children;
}

bool can_put_expression_in_current_bb(const struct cfg_builder_handler* handler) {
    assert(handler != NULL);

    const enum control_flow_type cf_type = handler->current_cf->type;
    const struct control_flow* const current_cf = handler->current_cf;
    const struct basic_block* const current_bb = handler->current_bb;

    bool continue_in_basic_block = false;

    switch (cf_type) {
        case CF_LINEAR: {
            continue_in_basic_block = true;
        } break;
        case CF_IF: {
            continue_in_basic_block = (current_bb == current_cf->pattern.cf_if->cond);
        } break;
        case CF_IF_ELSE: {
            continue_in_basic_block = (current_bb == current_cf->pattern.cf_if_else->cond);
        } break;
        case CF_WHILE: {
            continue_in_basic_block = (current_bb == current_cf->pattern.cf_while->cond);
        } break;
        case CF_DO_WHILE: {
            continue_in_basic_block = (current_bb == current_cf->pattern.cf_do_while->cond->entry);
        } break;
        case CF_FOR: {
            continue_in_basic_block =
                (current_bb == current_cf->pattern.cf_for->init)
                    || (current_bb == current_cf->pattern.cf_for->cond)
                    || (current_bb == current_cf->pattern.cf_for->iter->entry);
        } break;
        case CF_SWITCH: {
            continue_in_basic_block = (current_bb == current_cf->pattern.cf_switch->cond);
        } break;
        default: {
            continue_in_basic_block = false;
        } break;
    }

    return continue_in_basic_block;
}

void push_spelling_to_basic_block(CXCursor cursor, struct basic_block* bb) {
    str_t* expression_spelling = get_source_range_spelling(cursor);
    struct operation* operation = operation_new();
    operation_push_operand(operation, expression_spelling);
    basic_block_push_operation(bb, operation);
}