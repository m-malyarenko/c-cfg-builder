#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include <clang-c/Index.h>

#include "../utility/cursor_vec.h"
#include "../utility/basic_block_vec.h"
#include "../utility/spelling.h"
#include "../entity/basic_block.h"
#include "../entity/operation.h"
#include "cfg_sanitizer.h"
#include "cfg_builder.h"

struct cfg_builder_output build_function_cfg(CXCursor func_cursor) {
    if (clang_Cursor_isNull(func_cursor)
            || clang_isInvalid(clang_getCursorKind(func_cursor)))
    {
        return (struct cfg_builder_output) { NULL, NULL };
    }

    /* Root control flow with dummy basic block as terminator */
    struct cf_node* func = cf_func_new();
    struct basic_block* cfg_sink = basic_block_new();
    struct basic_block_vec* cfg_nodes = basic_block_vec_new();

    struct cfg_builder_handler handler = {
        .current_cf_node = NULL,
        .parent_cf_node = func,
        .return_cf_node = NULL,
        .break_target = NULL,
        .continue_target = NULL,
        .link_bb = cfg_sink,
        .sink_bb = cfg_sink,
        .cfg_nodes = cfg_nodes,
        .goto_table = goto_table_new(),
        .switch_handler = { NULL, NULL, NULL },
        .status = CFG_BUILDER_OK,
    };

    /* Function traversing entry */
    clang_visitChildren(func_cursor, visit_subnode, &handler);

    if (handler.status != CFG_BUILDER_OK) {
        cf_node_drop(&func);
        basic_block_vec_drop(&cfg_nodes);
        goto_table_drop(&handler.goto_table);
        return (struct cfg_builder_output) { NULL, NULL };
    }

    struct basic_block* cfg_entry = handler.link_bb;

    /* CFG post processing */
    connect_goto_labels(handler.goto_table);
    struct basic_block_vec* sanitized_cfg = NULL;
    
    sanitized_cfg = eliminate_empty_basic_blocks(cfg_nodes);
    basic_block_vec_drop(&cfg_nodes);
    cfg_nodes = sanitized_cfg;

    sanitized_cfg = merge_linear_basic_blocks(cfg_entry, cfg_sink);
    basic_block_vec_drop(&cfg_nodes);
    cfg_nodes = sanitized_cfg;

    /* Add sink node to CFG */
    basic_block_vec_push(cfg_nodes, cfg_sink);

    struct cfg_builder_output output = {
        .cfg_nodes = cfg_nodes,
        .cfg_sink = cfg_sink,
    };

    cf_node_drop(&func);
    goto_table_drop(&handler.goto_table);

    return output;
}

VISITOR(visit_subnode) {
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    struct cfg_builder_handler* handler = client_data;

    enum CXChildVisitResult result = CXChildVisit_Continue;
    
    /* Before call */
    struct cf_node* const saved_parent_cf_node = handler->parent_cf_node;
    struct cf_node* const saved_current_cf_node = handler->current_cf_node;

    handler->parent_cf_node = saved_current_cf_node;
    handler->current_cf_node = NULL;
    handler->return_cf_node = NULL;

    if (clang_isDeclaration(cursor_kind)) {
        handler->current_cf_node = cf_decl_new(handler->parent_cf_node);
        result = visit_declaration(cursor, parent, handler);
    } else if (clang_isExpression(cursor_kind)) {
        handler->current_cf_node = cf_expr_new(handler->parent_cf_node);
        result = visit_expression(cursor, parent, handler);
    } else if (clang_isStatement(cursor_kind)) {
        result = visit_statement(cursor, parent, handler);
    } else {
        handler->status = CFG_BUILDER_UNSUPPORTED_SYNTAX;
        handler->current_cf_node = NULL;
        result =  CXChildVisit_Break;
    }

    /* After call */
    handler->return_cf_node = handler->current_cf_node;
    handler->current_cf_node = saved_current_cf_node;
    handler->parent_cf_node = saved_parent_cf_node;

    return result;
}

VISITOR(visit_expression) {
    #ifndef NDEBUG
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    #endif
    struct cfg_builder_handler* handler = client_data;

    assert(clang_isExpression(cursor_kind));
    assert(handler->current_cf_node->type == CF_NODE_EXPR);

    struct cf_expr* const expr = handler->current_cf_node->node._expr;

    // TODO Traverse expression tree
    const struct cf_node* parent_node = handler->parent_cf_node;
    operation_t* expression_op = str_list_new();

    if (parent_node->type == CF_NODE_STMNT) {
        if ((parent_node->node._stmnt->type == CF_STMNT_SWITCH)
                && (parent_node->node._stmnt->_switch->select == PENDING_SUBNODE_MARKER))
        {
            str_list_push(expression_op, str_new("select = "));
        } else if ((parent_node->node._stmnt->type == CF_STMNT_CASE)
                        && (parent_node->node._stmnt->_case->const_expr == PENDING_SUBNODE_MARKER))
        {
            str_list_push(expression_op, str_new("select == "));
        } else if ((parent_node->node._stmnt->type == CF_STMNT_RETURN)
                        && (parent_node->node._stmnt->_return->return_expr == PENDING_SUBNODE_MARKER))
        {
            str_list_push(expression_op, str_new("ret "));
        }
    }

    str_list_push(expression_op, get_cursor_range_spelling(cursor));

    basic_block_push_operation(expr->bb, expression_op);
    basic_block_resize_links(expr->bb, 1);
    basic_block_set_link(expr->bb, handler->link_bb, 0);

    /* Add basic block to CFG nodes */
    basic_block_vec_push(handler->cfg_nodes, expr->bb);

    handler->current_cf_node->entry = expr->bb;
    handler->link_bb = expr->bb;

    return CXChildVisit_Continue;
}

VISITOR(visit_declaration) {
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    struct cfg_builder_handler* handler = client_data;

    assert(clang_isDeclaration(cursor_kind));
    assert(handler->current_cf_node->type == CF_NODE_DECL);

    if ((cursor_kind != CXCursor_VarDecl) && (cursor_kind != CXCursor_ParmDecl)) {
        handler->status = CFG_BUILDER_UNSUPPORTED_SYNTAX;
        handler->current_cf_node = NULL;
        return CXChildVisit_Break;
    }

    if (cursor_kind == CXCursor_ParmDecl) {
        return CXChildVisit_Continue;
    }

    struct cf_decl* const decl = handler->current_cf_node->node._decl;

    /* Type name */
    CXType cursor_type = clang_getCursorType(cursor);
    CXString cursor_type_spelling = clang_getTypeSpelling(cursor_type);
    str_append(decl->type_name, clang_getCString(cursor_type_spelling));
    clang_disposeString(cursor_type_spelling);

    decl->expr = cf_expr_new(handler->current_cf_node);
    push_spelling_to_basic_block(cursor, decl->expr->node._expr->bb);
    basic_block_resize_links(decl->expr->node._expr->bb, 1);
    basic_block_set_link(decl->expr->node._expr->bb, handler->link_bb, 0);
    basic_block_vec_push(handler->cfg_nodes, decl->expr->node._expr->bb);

    decl->expr->entry = decl->expr->node._expr->bb;
    handler->current_cf_node->entry = decl->expr->entry;
    handler->link_bb = decl->expr->entry;

    // /* Initialization */
    // struct cursor_vec* children = get_cursor_children(cursor);
    // assert((children->len == 1) || (children->len == 0));
    // const bool has_initialization = (children->len == 1);

    // enum CXChildVisitResult visit_result = CXChildVisit_Continue;

    // if (has_initialization) {
    //     visit_result = visit_subnode(children->buffer[0], cursor, handler);

    //     decl->expr = handler->return_cf_node;
    // } else {
    //     decl->expr = cf_expr_new(handler->current_cf_node);
    // }

    // handler->current_cf_node->entry =
    //     has_initialization
    //         ? decl->expr->entry
    //         : handler->link_bb;

    return CXChildVisit_Continue;
}

VISITOR(visit_statement) {
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    struct cfg_builder_handler* handler = client_data;

    assert(clang_isStatement(cursor_kind));

    enum CXChildVisitResult visit_result = CXChildVisit_Continue;

    switch (cursor_kind) {
        case CXCursor_UnexposedStmt: {
            const int visit_child_result =
                clang_visitChildren(cursor, visit_statement, handler);

            visit_result =
                (visit_child_result == 0)
                    ? CXChildVisit_Continue
                    : CXChildVisit_Break;
        } break;
        case CXCursor_DeclStmt: {
            handler->current_cf_node = cf_stmnt_decl_new(handler->parent_cf_node);
            visit_result = visit_declaration_statement(cursor, parent, handler);
        } break;
        case CXCursor_NullStmt: {
            handler->current_cf_node = cf_node_new(CF_NODE_NULL, NULL, handler->parent_cf_node);
            handler->current_cf_node->entry = handler->link_bb;
            visit_result = CXChildVisit_Continue;
        } break;
        case CXCursor_CompoundStmt: {
            handler->current_cf_node = cf_stmnt_compound_new(handler->parent_cf_node);
            visit_result = visit_compound_statement(cursor, parent, handler);
        } break;
        case CXCursor_IfStmt: {
            handler->current_cf_node = cf_stmnt_if_new(handler->parent_cf_node);
            visit_result = visit_if_statement(cursor, parent, handler);
        } break;
        case CXCursor_SwitchStmt: {
            handler->current_cf_node = cf_stmnt_switch_new(handler->parent_cf_node);
            visit_result = visit_switch_statement(cursor, parent, handler);
        } break;
        case CXCursor_WhileStmt: {
            handler->current_cf_node = cf_stmnt_while_new(handler->parent_cf_node);
            visit_result = visit_while_statement(cursor, parent, handler);
        } break;
        case CXCursor_DoStmt: {
            handler->current_cf_node = cf_stmnt_do_new(handler->parent_cf_node);
            visit_result = visit_do_statement(cursor, parent, handler);
        } break;
        case CXCursor_ForStmt: {
            handler->current_cf_node = cf_stmnt_for_new(handler->parent_cf_node);
            visit_result = visit_for_statement(cursor, parent, handler);
        } break;
        case CXCursor_BreakStmt: {
            handler->current_cf_node = cf_stmnt_new(CF_STMNT_BREAK, NULL, handler->parent_cf_node);
            visit_result = visit_break_statement(cursor, parent, handler);
        } break;
        case CXCursor_ContinueStmt: {
            handler->current_cf_node = cf_stmnt_new(CF_STMNT_CONTINUE, NULL, handler->parent_cf_node);
            visit_result = visit_continue_statement(cursor, parent, handler);
        } break;
        case CXCursor_ReturnStmt: {
            handler->current_cf_node = cf_stmnt_return_new(handler->parent_cf_node);
            visit_result = visit_return_statement(cursor, parent, handler);
        } break;
        case CXCursor_GotoStmt: {
            handler->current_cf_node = cf_stmnt_goto_new(handler->parent_cf_node);
            visit_result = visit_goto_statement(cursor, parent, handler);
        } break;
        case CXCursor_LabelStmt: {
            handler->current_cf_node = cf_stmnt_label_new(handler->parent_cf_node);
            visit_result = visit_label_statement(cursor, parent, handler);
        } break;
        case CXCursor_DefaultStmt:
        case CXCursor_CaseStmt: {
            handler->current_cf_node = cf_stmnt_case_new(handler->parent_cf_node);
            visit_result = visit_case_statement(cursor, parent, handler);
        } break;
        default: {
            handler->current_cf_node = NULL;
            handler->return_cf_node = NULL;
            handler->status = CFG_BUILDER_UNSUPPORTED_SYNTAX;
            visit_result = CXChildVisit_Break;
        } break;
    }

    return visit_result;
}

VISITOR(visit_compound_statement) {
    #ifndef NDEBUG
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    #endif
    struct cfg_builder_handler* handler = client_data;

    assert(cursor_kind == CXCursor_CompoundStmt);
    assert(handler->current_cf_node->type == CF_NODE_STMNT);
    assert(handler->current_cf_node->node._stmnt->type == CF_STMNT_COMPOUND);

    struct cursor_vec* children = get_cursor_children(cursor);
    const size_t children_num = children->len;

    struct cf_stmnt_compound* const stmnt_compound =
        handler->current_cf_node->node._stmnt->_compound;

    enum CXChildVisitResult visit_result = CXChildVisit_Continue;

    for (size_t i = 0; i < children_num; i++) {
        CXCursor child = children->buffer[(children_num - i) - 1];

        visit_result = visit_subnode(child, cursor, handler);

        if (visit_result == CXChildVisit_Break) {
            break;
        }

        cf_node_vec_push(stmnt_compound->items, handler->return_cf_node);
    }

    if (visit_result != CXChildVisit_Break) {
        handler->current_cf_node->entry =
            (children_num != 0)
                ? stmnt_compound->items->buffer[children_num - 1]->entry
                : handler->link_bb;
    } else {
        handler->current_cf_node = NULL;
    }

    cursor_vec_drop(&children);

    return visit_result;
}

VISITOR(visit_declaration_statement) {
    #ifndef NDEBUG
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    #endif
    struct cfg_builder_handler* handler = client_data;

    assert(cursor_kind == CXCursor_DeclStmt);

    struct cursor_vec* children = get_cursor_children(cursor);
    const size_t children_num = children->len;

    struct cf_stmnt_decl* const stmnt_decl =
        handler->current_cf_node->node._stmnt->_decl;

    enum CXChildVisitResult visit_result = CXChildVisit_Continue;

    for (size_t i = 0; i < children_num; i++) {
        CXCursor child = children->buffer[(children_num - i) - 1];

        visit_result = visit_subnode(child, cursor, handler);

        if (visit_result == CXChildVisit_Break) {
            break;
        }

        cf_node_vec_push(stmnt_decl->items, handler->return_cf_node);
    }

    handler->current_cf_node->entry =
        (children_num != 0)
            ? stmnt_decl->items->buffer[children_num - 1]->entry
            : handler->link_bb;
    
    cursor_vec_drop(&children);

    return visit_result;
}

VISITOR(visit_if_statement) {
    #ifndef NDEBUG
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    #endif
    struct cfg_builder_handler* handler = client_data;

    assert(cursor_kind == CXCursor_IfStmt);
    assert(handler->current_cf_node->type == CF_NODE_STMNT);
    assert(handler->current_cf_node->node._stmnt->type == CF_STMNT_IF);

    struct cursor_vec* children = get_cursor_children(cursor);
    const size_t children_num = children->len;
    assert((children_num == 2) || (children_num == 3));
    const bool has_else_branch = (children_num == 3);

    struct basic_block* const saved_link_bb = handler->link_bb;
    struct cf_stmnt_if* const stmnt_if = handler->current_cf_node->node._stmnt->_if;

    enum CXChildVisitResult visit_result = CXChildVisit_Continue;

    if (has_else_branch) {
        /* False branch */
        visit_result = visit_subnode(children->buffer[2], cursor, handler);

        if (visit_result == CXChildVisit_Break) {
            cursor_vec_drop(&children);
            return visit_result;
        }

        stmnt_if->body_false = handler->return_cf_node;
        handler->link_bb = saved_link_bb;
    } else {
        stmnt_if->body_false = NULL;
    }

    /* True branch */
    visit_result = visit_subnode(children->buffer[1], cursor, handler);

    if (visit_result == CXChildVisit_Break) {
        cursor_vec_drop(&children);
        return visit_result;
    }

    stmnt_if->body_true = handler->return_cf_node;

    /* Condition */
    visit_result = visit_subnode(children->buffer[0], cursor, handler);

    if (visit_result == CXChildVisit_Break) {
        cursor_vec_drop(&children);
        return visit_result;
    }

    stmnt_if->cond = handler->return_cf_node;

    connect_condition_branches(
        stmnt_if->cond,
        stmnt_if->body_true->entry,
        has_else_branch
            ? stmnt_if->body_false->entry
            : saved_link_bb
    );

    handler->current_cf_node->entry = stmnt_if->cond->entry;
    handler->link_bb = stmnt_if->cond->entry;

    cursor_vec_drop(&children);

    return visit_result;
}

VISITOR(visit_switch_statement) {
    #ifndef NDEBUG
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    #endif
    struct cfg_builder_handler* handler = client_data;

    assert(cursor_kind == CXCursor_SwitchStmt);

    struct cursor_vec* children = get_cursor_children(cursor);
    assert(children->len == 2);

    struct cf_stmnt_switch* const stmnt_switch =
        handler->current_cf_node->node._stmnt->_switch;

    enum CXChildVisitResult visit_result = CXChildVisit_Continue;

    struct basic_block* const saved_link_bb = handler->link_bb;
    struct basic_block* const saved_break_target = handler->break_target;
    struct basic_block* const saved_continue_target = handler->continue_target;

    /* Const expression */
    stmnt_switch->select = PENDING_SUBNODE_MARKER;
    visit_result = visit_subnode(children->buffer[0], cursor, handler);

    if (visit_result == CXChildVisit_Break) {
        cursor_vec_drop(&children);
        return visit_result;
    }

    stmnt_switch->select = handler->return_cf_node;
    handler->link_bb = saved_link_bb;

    /* Set break jump target */
    handler->break_target = saved_link_bb;
    handler->continue_target = NULL;

    /* Body */
    visit_result = visit_subnode(children->buffer[1], cursor, handler);

    if (visit_result == CXChildVisit_Break) {
        cursor_vec_drop(&children);
        return visit_result;
    }

    stmnt_switch->body = handler->return_cf_node;

    connect_tail(stmnt_switch->select, stmnt_switch->body->entry);

    /* Handle default statemnet */
    struct cf_node* const default_node = handler->switch_handler.default_node;
    struct cf_node* const last_case_node = handler->switch_handler.last_case_node;
 
    if ((default_node != NULL) && (default_node != last_case_node)) {
        basic_block_resize_links(last_case_node->node._stmnt->_case->imag_bb, 1);
        basic_block_set_link(last_case_node->node._stmnt->_case->imag_bb, default_node->entry, 0);
    }

    handler->current_cf_node->entry = stmnt_switch->select->entry;
    handler->link_bb = stmnt_switch->select->entry;
    handler->break_target = saved_break_target;
    handler->continue_target = saved_continue_target;
    handler->switch_handler.default_node = NULL;
    handler->switch_handler.last_case_node = NULL;

    cursor_vec_drop(&children);
    
    return visit_result;
}

VISITOR(visit_while_statement) {
    #ifndef NDEBUG
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    #endif
    struct cfg_builder_handler* handler = client_data;

    assert(cursor_kind == CXCursor_WhileStmt);

    struct cursor_vec* children = get_cursor_children(cursor);
    assert(children->len == 2);

    struct basic_block* const saved_link_bb = handler->link_bb;
    struct basic_block* const saved_break_target = handler->break_target;
    struct basic_block* const saved_continue_target = handler->continue_target;

    struct cf_stmnt_while* const stmnt_while =
        handler->current_cf_node->node._stmnt->_while;

    enum CXChildVisitResult visit_result = CXChildVisit_Continue;

    /* Condition */
    visit_result = visit_subnode(children->buffer[0], cursor, handler);

    if (visit_result == CXChildVisit_Break) {
        cursor_vec_drop(&children);
        return visit_result;
    }

    stmnt_while->cond = handler->return_cf_node;
    handler->link_bb = stmnt_while->cond->entry;

    /* Set loop jump targets */
    handler->break_target = saved_link_bb;
    handler->continue_target = stmnt_while->cond->entry;

    /* Body branch */
    visit_result = visit_subnode(children->buffer[1], cursor, handler);

    if (visit_result == CXChildVisit_Break) {
        cursor_vec_drop(&children);
        return visit_result;
    }

    stmnt_while->body = handler->return_cf_node;

    connect_condition_branches(
        stmnt_while->cond,
        stmnt_while->body->entry,
        saved_link_bb
    );

    handler->current_cf_node->entry = stmnt_while->cond->entry;
    handler->link_bb = stmnt_while->cond->entry;
    handler->break_target = saved_break_target;
    handler->continue_target = saved_continue_target;

    cursor_vec_drop(&children);

    return visit_result;
}

VISITOR(visit_do_statement) {
    #ifndef NDEBUG
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    #endif
    struct cfg_builder_handler* handler = client_data;

    assert(cursor_kind == CXCursor_DoStmt);

    struct cursor_vec* children = get_cursor_children(cursor);
    assert(children->len == 2);

    struct basic_block* const saved_link_bb = handler->link_bb;
    struct basic_block* const saved_break_target = handler->break_target;
    struct basic_block* const saved_continue_target = handler->continue_target;

    struct cf_stmnt_do* const stmnt_do =
        handler->current_cf_node->node._stmnt->_do;

    enum CXChildVisitResult visit_result = CXChildVisit_Continue;

    /* Condition */
    visit_result = visit_subnode(children->buffer[1], cursor, handler);

    if (visit_result == CXChildVisit_Break) {
        cursor_vec_drop(&children);
        return visit_result;
    }

    stmnt_do->cond = handler->return_cf_node;

    /* Set loop jump targets */
    handler->break_target = saved_link_bb;
    handler->continue_target = stmnt_do->cond->entry;

    /* Body branch */
    handler->link_bb = stmnt_do->cond->entry;
    visit_result = visit_subnode(children->buffer[0], cursor, handler);

    if (visit_result == CXChildVisit_Break) {
        cursor_vec_drop(&children);
        return visit_result;
    }

    stmnt_do->body = handler->return_cf_node;

    connect_condition_branches(
        stmnt_do->cond,
        stmnt_do->body->entry,
        saved_link_bb
    );

    handler->current_cf_node->entry = stmnt_do->body->entry;
    handler->link_bb = stmnt_do->body->entry;
    handler->break_target = saved_break_target;
    handler->continue_target = saved_continue_target;

    cursor_vec_drop(&children);

    return visit_result;
}

VISITOR(visit_for_statement) {
    #ifndef NDEBUG
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    #endif
    struct cfg_builder_handler* handler = client_data;

    assert(cursor_kind == CXCursor_ForStmt);

    struct cursor_vec* children = get_cursor_children(cursor);
    const size_t children_num = children->len;
    assert(children_num <= 4);

    if (children_num != 4) {
        handler->status = CFG_BUILDER_UNSUPPORTED_SYNTAX;
        handler->current_cf_node = NULL;
        return CXChildVisit_Break;
    }

    struct basic_block* const saved_link_bb = handler->link_bb;
    struct basic_block* const saved_break_target = handler->break_target;
    struct basic_block* const saved_continue_target = handler->continue_target;

    struct cf_stmnt_for* const stmnt_for =
        handler->current_cf_node->node._stmnt->_for;

    enum CXChildVisitResult visit_result = CXChildVisit_Continue;

    /* Condition */
    handler->link_bb = NULL;
    visit_result = visit_subnode(children->buffer[1], cursor, handler);

    if (visit_result == CXChildVisit_Break) {
        cursor_vec_drop(&children);
        return visit_result;
    }

    stmnt_for->cond = handler->return_cf_node;

    /* Initialization */
    handler->link_bb = stmnt_for->cond->entry;
    visit_result = visit_subnode(children->buffer[0], cursor, handler);

    if (visit_result == CXChildVisit_Break) {
        cursor_vec_drop(&children);
        return visit_result;
    }

    stmnt_for->init = handler->return_cf_node;

    /* Iteration */
    handler->link_bb = stmnt_for->cond->entry;
    visit_result = visit_subnode(children->buffer[2], cursor, handler);

    if (visit_result == CXChildVisit_Break) {
        cursor_vec_drop(&children);
        return visit_result;
    }

    stmnt_for->iter = handler->return_cf_node;

    /* Set loop jump targets */
    handler->break_target = saved_link_bb;
    handler->continue_target = stmnt_for->iter->entry;

    /* Body */
    handler->link_bb = stmnt_for->iter->entry;
    visit_result = visit_subnode(children->buffer[3], cursor, handler);

    if (visit_result == CXChildVisit_Break) {
        cursor_vec_drop(&children);
        return visit_result;
    }

    stmnt_for->body = handler->return_cf_node;

    connect_condition_branches(
        stmnt_for->cond,
        stmnt_for->body->entry,
        saved_link_bb
    );

    handler->current_cf_node->entry = stmnt_for->init->entry;
    handler->link_bb = stmnt_for->init->entry;
    handler->break_target = saved_break_target;
    handler->continue_target = saved_continue_target;

    cursor_vec_drop(&children);

    return visit_result;
}

VISITOR(visit_break_statement) {
    #ifndef NDEBUG
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    #endif
    struct cfg_builder_handler* handler = client_data;

    assert(cursor_kind == CXCursor_BreakStmt);

    handler->link_bb = handler->break_target;
    handler->current_cf_node->entry = handler->break_target;

    return CXChildVisit_Continue;
}

VISITOR(visit_continue_statement) {
    #ifndef NDEBUG
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    #endif
    struct cfg_builder_handler* handler = client_data;

    assert(cursor_kind == CXCursor_ContinueStmt);

    handler->current_cf_node->entry = handler->continue_target;
    handler->link_bb = handler->continue_target;

    return CXChildVisit_Continue;
}

VISITOR(visit_return_statement) {
    #ifndef NDEBUG
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    #endif
    struct cfg_builder_handler* handler = client_data;

    assert(cursor_kind == CXCursor_ReturnStmt);

    struct cursor_vec* children = get_cursor_children(cursor);
    const size_t children_num = children->len;
    assert((children_num == 0) || (children_num == 1));
    const bool has_expression = (children_num == 1);

    struct cf_stmnt_return* const stmnt_return =
        handler->current_cf_node->node._stmnt->_return;

    enum CXChildVisitResult visit_result = CXChildVisit_Continue;

    if (has_expression) {
        stmnt_return->return_expr = PENDING_SUBNODE_MARKER;
        visit_result = visit_subnode(children->buffer[0], cursor, handler);

        if (visit_result == CXChildVisit_Break) {
            cursor_vec_drop(&children);
            return visit_result;
        }

        stmnt_return->return_expr = handler->return_cf_node;
        connect_tail(stmnt_return->return_expr, handler->sink_bb);
        handler->current_cf_node->entry = stmnt_return->return_expr->entry;
    } else {
        stmnt_return->return_expr = NULL;
        handler->current_cf_node->entry = handler->sink_bb;
        handler->link_bb = handler->sink_bb;
    }

    cursor_vec_drop(&children);

    return visit_result;
}

VISITOR(visit_case_statement) {
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    struct cfg_builder_handler* handler = client_data;

    assert((cursor_kind == CXCursor_CaseStmt) || (cursor_kind == CXCursor_DefaultStmt));
    const bool is_default = (cursor_kind == CXCursor_DefaultStmt);

    struct cursor_vec* children = get_cursor_children(cursor);
    if (is_default) {
        assert((children->len == 0) || (children->len == 1));
    } else {
        assert((children->len == 1) || (children->len == 2));
    }

    const bool has_body = is_default ? (children->len == 1) : (children->len == 2);
    const size_t body_child_idx = is_default ? 0 : 1;

    struct cf_stmnt_case* const stmnt_case =
        handler->current_cf_node->node._stmnt->_case;

    enum CXChildVisitResult visit_result = CXChildVisit_Continue;

    struct basic_block* const saved_link_bb = handler->link_bb;

    if (has_body) {
        /* Body */
        stmnt_case->body = PENDING_SUBNODE_MARKER;
        visit_result = visit_subnode(children->buffer[body_child_idx], cursor, handler);

        if (visit_result == CXChildVisit_Break) {
            cursor_vec_drop(&children);
            return visit_result;
        }

        stmnt_case->body = handler->return_cf_node;       
    }

    if (!is_default) {
        /* Constant expression condition */
        stmnt_case->const_expr = PENDING_SUBNODE_MARKER;
        visit_result = visit_subnode(children->buffer[0], cursor, handler);

        if (visit_result == CXChildVisit_Break) {
            cursor_vec_drop(&children);
            return visit_result;
        }

        stmnt_case->const_expr = handler->return_cf_node;
    } else {
        stmnt_case->const_expr = NULL;
    }

    const bool is_last_case = (handler->switch_handler.last_case_node == NULL);
    struct basic_block* next_case_link = NULL;

    if (is_last_case) {
        next_case_link = saved_link_bb;
    } else {
        next_case_link =
            (handler->switch_handler.prev_case_node != NULL)
                ? handler->switch_handler.prev_case_node->entry
                : handler->switch_handler.default_node->entry;
    }

    basic_block_resize_links(stmnt_case->imag_bb, 1);
    basic_block_set_link(stmnt_case->imag_bb, next_case_link, 0);
    basic_block_vec_push(handler->cfg_nodes, stmnt_case->imag_bb);

    if (!is_default) {
        connect_condition_branches(
            stmnt_case->const_expr,
            has_body ? stmnt_case->body->entry : saved_link_bb,
            stmnt_case->imag_bb
        );

        handler->current_cf_node->entry = stmnt_case->const_expr->entry;
    } else {
        handler->current_cf_node->entry =
            has_body
                ? stmnt_case->body->entry
                : stmnt_case->imag_bb;
    }

    if (is_last_case) {
        handler->switch_handler.last_case_node = handler->current_cf_node;
    }

    if (is_default) {
        handler->switch_handler.default_node = handler->current_cf_node;
    } else {
        handler->switch_handler.prev_case_node = handler->current_cf_node;
    }

    handler->link_bb = has_body ? stmnt_case->body->entry : saved_link_bb;

    return visit_result;
}

VISITOR(visit_goto_statement) {
    #ifndef NDEBUG
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    #endif
    struct cfg_builder_handler* handler = client_data;

    assert(cursor_kind == CXCursor_GotoStmt);

    struct cf_stmnt_goto* const stmnt_goto =
        handler->current_cf_node->node._stmnt->_goto;

    /* Get destination label */
    struct cursor_vec* children = get_cursor_children(cursor);
    assert(children->len == 1);

    /* Save label */
    CXString label = clang_getCursorDisplayName(children->buffer[0]);
    str_append(stmnt_goto->label, clang_getCString(label));
    clang_disposeString(label);

    /* Connect imaginary basic block to link */
    basic_block_resize_links(stmnt_goto->imag_bb, 1);
    basic_block_set_link(stmnt_goto->imag_bb, handler->link_bb, 0);

    /* Add imaginary basic block to CFG nodes */
    basic_block_vec_push(handler->cfg_nodes, stmnt_goto->imag_bb);

    /* Add source entry to goto table */
    goto_table_push(handler->goto_table, handler->current_cf_node, GOTO_ENTRY_SRC);

    handler->current_cf_node->entry = stmnt_goto->imag_bb;
    handler->link_bb = stmnt_goto->imag_bb;

    cursor_vec_drop(&children);

    return CXChildVisit_Continue;
}

VISITOR(visit_label_statement) {
    #ifndef NDEBUG
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    #endif
    struct cfg_builder_handler* handler = client_data;

    assert(cursor_kind == CXCursor_LabelStmt);

    struct cf_stmnt_label* const stmnt_label =
        handler->current_cf_node->node._stmnt->_label;

    /* Save label */
    CXString label = clang_getCursorDisplayName(cursor);
    str_append(stmnt_label->label, clang_getCString(label));
    clang_disposeString(label);

    /* Get label statement body */
    struct cursor_vec* children = get_cursor_children(cursor);
    assert((children->len == 1) || (children->len == 0));
    const bool has_body = (children->len == 1);

    if (has_body) {
        const enum CXChildVisitResult visit_result =
            visit_subnode(children->buffer[0], cursor, handler);

        if (visit_result == CXChildVisit_Break) {
            cursor_vec_drop(&children);
            return visit_result;
        }
    }

    /* Add destination entry to goto table */
    goto_table_push(handler->goto_table, handler->current_cf_node, GOTO_ENTRY_DEST);

    handler->current_cf_node->entry =
        has_body
            ? handler->return_cf_node->entry
            : handler->link_bb;

    cursor_vec_drop(&children);

    return CXChildVisit_Continue;
}

VISITOR(inspect_children) {
    struct cursor_vec* children = client_data;
    cursor_vec_push(children, cursor);

    return CXChildVisit_Continue;
}

void connect_goto_labels(struct goto_table* table) {
    if (table == NULL) {
        return;
    }

    const size_t src_num = table->src->len;
    const size_t dest_num = table->dest->len;

    for (size_t i = 0; i < src_num; i++) {
        struct cf_node* src_entry = table->src->buffer[i];
        struct basic_block* const imag_bb = src_entry->node._stmnt->_goto->imag_bb;
        assert(imag_bb->links.len == 1);
        const str_t* src_label = src_entry->node._stmnt->_goto->label;

        for (size_t j = 0; j < dest_num; j++) {
            const struct cf_node* dest_entry = table->dest->buffer[j];
            const str_t* dest_label = dest_entry->node._stmnt->_label->label;

            if (str_eq(src_label, dest_label)) {
                basic_block_set_link(imag_bb, dest_entry->entry, 0);
            }
        }
    }
}

struct cursor_vec* get_cursor_children(CXCursor cursor) {
    struct cursor_vec* children = cursor_vec_new();
    clang_visitChildren(cursor, inspect_children, children);

    return children;
}

void connect_tail(struct cf_node* node, struct basic_block* target) {
    assert(node != NULL);
    assert(target != NULL);

    // TODO Support all types of nodes
    assert((node->type == CF_NODE_EXPR) || (node->type = CF_NODE_DECL));

    struct cf_expr* expr =
        (node->type == CF_NODE_DECL)
            ? node->node._decl->expr->node._expr
            : node->node._expr;

    basic_block_resize_links(expr->bb, 1);
    basic_block_set_link(expr->bb, target, 0); 
}

void connect_condition_branches(struct cf_node* cond,
                                struct basic_block* true_entry,
                                struct basic_block* false_entry)
{
    assert(cond != NULL);
    assert(true_entry != NULL);
    assert(false_entry != NULL);
    assert(cond->type == CF_NODE_EXPR);

    struct cf_expr* const cond_expr = cond->node._expr;
    basic_block_resize_links(cond_expr->bb, 2); 
    basic_block_set_link(cond_expr->bb, true_entry, 0);
    basic_block_set_link(cond_expr->bb, false_entry, 1);
}

void push_spelling_to_basic_block(CXCursor cursor, struct basic_block* bb) {
    str_t* spelling = get_cursor_range_spelling(cursor);
    operation_t* operation = str_list_with_capacity(1);
    str_list_push(operation, spelling);
    basic_block_push_operation(bb, operation);
}
