#include <stdlib.h>
#include <stdio.h>

#include <clang-c/Index.h>
#include <ustring/str.h>

#include "builder/cfg_builder.h"
#include "converter/dot_converter.h"

#define MAX_MAIN_ARGC ((int) 3)
#define SRC_FILE_ARGV_IDX ((int) 1)
#define DOT_FILE_ARGV_IDX ((int) 2)

VISITOR(find_tu_function);

VISITOR(print_visitor);

int main(int argc, char* argv[]) {
    if (argc > MAX_MAIN_ARGC) {
        fprintf(stderr, "Incorrect number of arguments\n");
        return EXIT_FAILURE;
    }

    CXIndex index = clang_createIndex(0, 0);

    CXTranslationUnit tu =
        clang_createTranslationUnitFromSourceFile(
            index,
            argv[SRC_FILE_ARGV_IDX],
            0, NULL,
            0, NULL
        );

    if (tu == NULL) {
        fprintf(stderr, "Failed to create AST from source\n");
        clang_disposeIndex(index);
        return EXIT_FAILURE;
    }
    CXCursor root_cursor = clang_getTranslationUnitCursor(tu);

    CXCursor function_cursor;
    const int visit_status =
        clang_visitChildren(root_cursor, find_tu_function, &function_cursor);

    if (visit_status == 0) {
        fprintf(stderr, "No function found in translation unit\n");
        clang_disposeTranslationUnit(tu);
        clang_disposeIndex(index);
        return EXIT_FAILURE;
    }

    #ifndef NDEBUG
    clang_visitChildren(function_cursor, print_visitor, NULL);
    #endif

    struct cfg_builder_output cfg = build_function_cfg(function_cursor);

    if ((cfg.cfg_nodes == NULL) && (cfg.cfg_sink == NULL)) {
        fprintf(stderr, "Failed to build function CFG\n");
        clang_disposeTranslationUnit(tu);
        clang_disposeIndex(index);
        return EXIT_FAILURE;
    }

    str_t* dot_graph_str = convert_to_dot(cfg);

    FILE* dot_file =
        (argc == 3)
            ? fopen(argv[DOT_FILE_ARGV_IDX], "w")
            : stdout;

    if (dot_file == NULL) {
        perror("Failed to open DOT file");
        clang_disposeTranslationUnit(tu);
        clang_disposeIndex(index);
        return EXIT_FAILURE;
    }

    fprintf(dot_file, "%s", str_as_ptr(dot_graph_str));
    fclose(dot_file);

    str_drop(&dot_graph_str);
    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);

    return EXIT_SUCCESS;
}

VISITOR(find_tu_function) {
    if (clang_getCursorKind(cursor) == CXCursor_FunctionDecl) {
        *((CXCursor*) client_data) = cursor;
        return CXVisit_Break;
    } else {
        return CXVisit_Continue;
    }
}

VISITOR(print_visitor) {
    CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);
    CXSourceLocation sl = clang_getCursorLocation(cursor);

    const enum CXCursorKind kind = clang_getCursorKind(cursor);
    CXToken* token = clang_getToken(tu, sl);

    CXString kind_spelling = clang_getCursorKindSpelling(kind);
    CXString cursor_spelling = clang_getCursorSpelling(cursor);
    CXString token_spelling = clang_getTokenSpelling(tu, *token);

    str_t* parent_ident = client_data;
    str_t* ident = str_copy(parent_ident);
    str_append(ident, "\t");

    str_t* category_name = str_new(NULL);

    if (clang_isDeclaration(kind)) {
        str_append(category_name, "Declaration");
    } else if (clang_isExpression(kind)) {
        str_append(category_name, "Expression");
    } else if (clang_isStatement(kind)) {
        str_append(category_name, "Statement");
    } else {
        str_append(category_name, "Other");
    }

    printf("%s%s| Kind: %s, Name: %s, Token: %s\n",
        str_as_ptr(ident),
        str_as_ptr(category_name),
        clang_getCString(kind_spelling),
        clang_getCString(cursor_spelling),
        clang_getCString(token_spelling)
    );

    clang_visitChildren(cursor, print_visitor, ident);

    clang_disposeString(kind_spelling);
    clang_disposeString(cursor_spelling);
    clang_disposeString(token_spelling);
    str_drop(&ident);

    return CXChildVisit_Continue;
}
