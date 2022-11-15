#include <stdlib.h>
#include <stdio.h>

#include <clang-c/Index.h>

#define MAIN_ARGC ((int) 2)
#define SRC_FILE_ARGV_IDX ((int) 1)

enum CXChildVisitResult print_visitor(CXCursor cursor,
                                      CXCursor parent,
                                      CXClientData client_data)
{
    CXString kind_spelling =
        clang_getCursorKindSpelling(clang_getCursorKind(cursor));

    CXString cursor_spelling = clang_getCursorSpelling(cursor);

    printf("Kind: %s, Name: %s\n",
        clang_getCString(kind_spelling),
        clang_getCString(cursor_spelling)
    );

    clang_disposeString(kind_spelling);

    return CXChildVisit_Recurse;
}

enum CXChildVisitResult find_tu_function(CXCursor cursor,
                                         CXCursor parent,
                                         CXClientData client_data)
{
    if (clang_getCursorKind(cursor) == CXCursor_FunctionDecl) {
        *((CXCursor*) client_data) = cursor;
        return CXVisit_Break;
    } else {
        return CXVisit_Continue;
    }
}

int main(int argc, char* argv[]) {
    if (argc != MAIN_ARGC) {
        fprintf(stderr, "Incorrect number of arguments\n");
        return EXIT_FAILURE;
    }

    CXIndex index = clang_createIndex(0, 0);

    fprintf(stdout, "Building AST from source file '%s'...\n", argv[SRC_FILE_ARGV_IDX]);

    CXTranslationUnit tu =
        clang_createTranslationUnitFromSourceFile(
            index,
            argv[SRC_FILE_ARGV_IDX],
            0, NULL,
            0, NULL
        );

    if (tu == NULL) {
        fprintf(stderr, "Dailed to create AST from source\n");
        clang_disposeIndex(index);
        return EXIT_FAILURE;
    } else {
        fprintf(stdout, "Done\n");
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

    clang_visitChildren(function_cursor, print_visitor, NULL);

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);

    return EXIT_SUCCESS;
}
