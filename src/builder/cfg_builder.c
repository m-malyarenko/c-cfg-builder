#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include <clang-c/Index.h>

#include "../utility/cursor_vec.h"
#include "../utility/basic_block_vec.h"
#include "../utility/spelling.h"
#include "../entity/basic_block.h"
#include "../entity/operation.h"
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
        .status = CFG_BUILDER_OK,
    };

    /* Function traversing entry */
    // clang_visitChildren(func_cursor, visit_subnode, &handler);

    basic_block_vec_push(cfg_nodes, cfg_sink);

    /* CFG post processing */
    connect_goto_labels(handler.goto_table);
    goto_table_drop(&handler.goto_table);

    if (handler.status != CFG_BUILDER_OK) {
        cf_node_drop(&func);
        basic_block_vec_drop(&cfg_nodes);
        return (struct cfg_builder_output) { NULL, NULL };
    }

    struct cfg_builder_output output = {
        .cfg_nodes = cfg_nodes,
        .cfg_sink = cfg_sink,
    };

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
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    struct cfg_builder_handler* handler = client_data;

    assert(clang_isExpression(cursor_kind));
    assert(handler->current_cf_node->type == CF_NODE_EXPR);

    struct cf_expr* const expr = handler->current_cf_node->node._expr;

    // TODO Traverse expression tree
    push_spelling_to_basic_block(cursor, expr->bb);
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

    struct cf_decl* const decl = handler->current_cf_node->node._decl;

    /* Type name */
    CXType cursor_type = clang_getCursorType(cursor);
    CXString cursor_type_spelling = clang_getTypeSpelling(cursor_type);
    str_append(decl->type_name, clang_getCString(cursor_type_spelling));
    clang_disposeString(cursor_type_spelling);

    /* Initialization */
    struct cursor_vec* children = get_cursor_children(cursor);
    assert((children->len == 1) || (children->len == 0));
    const bool has_initialization = (children->len == 1);

    enum CXChildVisitResult visit_result = CXChildVisit_Continue;

    if (has_initialization) {
        visit_result = visit_subnode(children->buffer[0], cursor, handler);

        decl->expr = handler->return_cf_node;
    } else {
        decl->expr = cf_expr_new(handler->current_cf_node);
    }

    handler->current_cf_node->entry =
        has_initialization
            ? decl->expr->entry
            : handler->link_bb;

    return visit_result;
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
        case CXCursor_CaseStmt: {
            handler->current_cf_node = cf_stmnt_case_new(handler->parent_cf_node);
            visit_result = visit_case_statement(cursor, parent, handler);
        } break;
        case CXCursor_DefaultStmt: {
            handler->current_cf_node = cf_stmnt_default_new(handler->parent_cf_node);
            visit_result = visit_default_statement(cursor, parent, handler);
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
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
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
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
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
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
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
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    struct cfg_builder_handler* handler = client_data;

    assert(cursor_kind == CXCursor_SwitchStmt);

    struct cursor_vec* children = get_cursor_children(cursor);
    assert((children->len == 1) || (children->len == 2));
    const bool has_body = (children->len == 2);

    struct cf_stmnt_case* const cf_stmnt_switch =
        handler->current_cf_node->node._stmnt->_switch;

    enum CXChildVisitResult visit_result = CXChildVisit_Continue;

    struct basic_block* const saved_link_bb = handler->link_bb;

    /* Const expression */
    visit_result = visit_subnode(children->buffer[0], cursor, handler);

    if (visit_result == CXChildVisit_Break) {
        cursor_vec_drop(&children);
        return visit_result;
    }

    cf_stmnt_switch->const_expr = handler->return_cf_node;

    cursor_vec_drop(&children);
    
    return CXChildVisit_Continue;
}

VISITOR(visit_while_statement) {
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
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
    handler->continue_target = stmnt_while->cond->entry;

    /* Body branch */
    handler->break_target = saved_link_bb;
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
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
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
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
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
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    struct cfg_builder_handler* handler = client_data;

    assert(cursor_kind == CXCursor_BreakStmt);

    handler->link_bb = handler->break_target;
    handler->current_cf_node->entry = handler->break_target;

    return CXChildVisit_Continue;
}

VISITOR(visit_continue_statement) {
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    struct cfg_builder_handler* handler = client_data;

    assert(cursor_kind == CXCursor_ContinueStmt);

    handler->current_cf_node->entry = handler->continue_target;
    handler->link_bb = handler->continue_target;

    return CXChildVisit_Continue;
}

VISITOR(visit_return_statement) {
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
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

    assert(cursor_kind == CXCursor_CaseStmt);

    return CXChildVisit_Continue;
}

VISITOR(visit_default_statement) {
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
    struct cfg_builder_handler* handler = client_data;

    assert(cursor_kind == CXCursor_DefaultStmt);
    struct cursor_vec* children = get_cursor_children(cursor);
    assert((children->len == 0) || (children->len == 1));
    const bool has_body = (children->len == 1);

    struct basic_block* const saved_link_bb = handler->link_bb;

    struct cf_stmnt_case* const stmnt_default =
        handler->current_cf_node->node._stmnt->_default;

    enum CXChildVisitResult visit_result = CXChildVisit_Continue;

    if (has_body) {
        /* Body */
        handler->link_bb = handler->break_target; // ?
        visit_result = visit_subnode(children->buffer[0], cursor, handler);

        if (visit_result == CXChildVisit_Break) {
            cursor_vec_drop(&children);
            return visit_result;
        }

        stmnt_default->body = handler->return_cf_node;
    } else {
        stmnt_default->body = NULL;
    }

    // TODO Сделать default ветку по мотивам goto: приделать её после последнего case выражения.

    /* Const expression */
    visit_result = visit_subnode(children->buffer[0], cursor, handler);

    if (visit_result == CXChildVisit_Break) {
        cursor_vec_drop(&children);
        return visit_result;
    }

    stmnt_default->const_expr = handler->return_cf_node;
    handler->current_cf_node->entry = stmnt_default->const_expr->entry;

    cursor_vec_drop(&children);
}

VISITOR(visit_goto_statement) {
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
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
    push_spelling_to_basic_block(cursor, stmnt_goto->imag_bb);
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
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);
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

struct cursor_vec* normalize_switch_body(CXCursor cursor) {
    const enum CXCursorKind cursor_kind = clang_getCursorKind(cursor);

    assert(cursor_kind == CXCursor_SwitchStmt);


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
    str_t* expression_spelling = get_source_range_spelling(cursor);
    struct operation* operation = operation_new();
    operation_push_operand(operation, expression_spelling);
    basic_block_push_operation(bb, operation);
}
