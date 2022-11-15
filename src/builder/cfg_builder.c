#include <stdlib.h>

#include <clang-c/Index.h>

#include "../entity/function.h"
#include "cfg_builder.h"

const struct function build_function_cfg(CXCursor function_cursor) {
    if (clang_Cursor_isNull(function_cursor)
            || clang_isInvalid(clang_getCursorKind(function_cursor)))
    {
        return ENTITY_FUNCTION_NULL();
    }

    struct function f = ENTITY_FUNCTION_NEW();

    struct cfg_builder_handler handler = {
        .current_control_flow = control_flow_new(CONTROL_FLOW_LINEAR, f.source, f.sink)
    };

    return f;
}

VISITOR(visit_function) {
    const enum CXCursorKind kind = clang_getCursorKind(cursor);

    if (kind == CXCursor_ParmDecl) {
        return CXVisit_Continue;
    }

    int visit_status = 0;

    if (clang_isDeclaration(kind)) {
        visit_status = clang_visitChildren(cursor, visit_declaration, NULL);
    } else if (clang_isExpression(kind)) {
        visit_status = clang_visitChildren(cursor, visit_expresstion, NULL);
    } else if (clang_isStatement(kind)) {
        visit_status = clang_visitChildren(cursor, visit_statement, NULL);
    }

    return CXChildVisit_Continue;
}

VISITOR(visit_declaration) {
    return CXChildVisit_Continue;
}

VISITOR(visit_statement) {
    return CXChildVisit_Continue;
}

VISITOR(visit_expresstion) {
    return CXChildVisit_Continue;
}
