#ifndef __CFG_CFG_BUILDER_H__
#define __CFG_CFG_BUILDER_H__

#include <stdbool.h>

#include <clang-c/Index.h>
#include <ustring/str_list.h>

#include "../entity/basic_block.h"
#include "../utility/basic_block_vec.h"
#include "../utility/goto_table.h"
#include "control_flow/cf_node.h"

#define VISITOR(name) enum CXChildVisitResult name(CXCursor cursor, CXCursor parent, CXClientData client_data)

enum cfg_builder_status {
    CFG_BUILDER_OK,
    CFG_BUILDER_UNSUPPORTED_SYNTAX,
};

struct cfg_builder_output {
    struct basic_block_vec* cfg_nodes;
    struct basic_block* cfg_sink;
};

struct cfg_builder_handler {
    struct cf_node* parent_cf_node;         /** Parent control flow node */
    struct cf_node* current_cf_node;        /** Current control flow node */
    struct cf_node* return_cf_node;         /** Control flow node return to parent */
    struct basic_block* break_target;       /** Current target for `break` statement */
    struct basic_block* continue_target;    /** Current target for `continue` statement */
    struct basic_block* link_bb;            /** Link basic block from previous control flow node */
    struct basic_block_vec* cfg_nodes;      /** Accumulator for CFG basic blocks */
    struct basic_block* sink_bb;            /** Dummy sink basic block as function terminator */
    struct goto_table* goto_table;          /** Table to connect goto source & destination labels */
    enum cfg_builder_status status;         /** Status of CFG builer */
};

/* Entry point */

struct cfg_builder_output build_function_cfg(CXCursor func_cursor);

/* Visitors */

VISITOR(visit_subnode);

VISITOR(visit_expression);

VISITOR(visit_declaration);

VISITOR(visit_statement);

VISITOR(visit_compound_statement);

VISITOR(visit_declaration_statement);

VISITOR(visit_if_statement);

VISITOR(visit_switch_statement);

VISITOR(visit_while_statement);

VISITOR(visit_do_statement);

VISITOR(visit_for_statement);

VISITOR(visit_break_statement);

VISITOR(visit_continue_statement);

VISITOR(visit_return_statement);

VISITOR(visit_case_statement);

VISITOR(visit_default_statement);

VISITOR(visit_goto_statement);

VISITOR(visit_label_statement);

VISITOR(inspect_children);

/* Switch handle */

struct cursor_vec* normalize_switch_body(CXCursor cursor);

/* Goto handle */

void connect_goto_labels(struct goto_table* table);

/* Utility */

struct cursor_vec* get_cursor_children(CXCursor cursor);

void connect_tail(struct cf_node* node, struct basic_block* target);

void connect_condition_branches(struct cf_node* cond,
                                struct basic_block* true_entry,
                                struct basic_block* false_entry);

void push_spelling_to_basic_block(CXCursor cursor, struct basic_block* bb);

#endif /* __CFG_CFG_BUILDER_H__ */
