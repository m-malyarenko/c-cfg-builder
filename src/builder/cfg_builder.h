#ifndef __CFG_CFG_BUILDER_H__
#define __CFG_CFG_BUILDER_H__

#include <clang-c/Index.h>

#include "control_flow/control_flow.h"
#include "../entity/function.h"

#define VISITOR(name) enum CXChildVisitResult name(CXCursor cursor, CXCursor parent, CXClientData client_data)

struct cfg_builder_handler {
    struct control_flow* current_control_flow;
};

const struct function build_function_cfg(CXCursor function_cursor);

VISITOR(visit_function);

VISITOR(visit_declaration);

VISITOR(visit_expresstion);

VISITOR(visit_statement);

#endif /* __CFG_CFG_BUILDER_H__ */
