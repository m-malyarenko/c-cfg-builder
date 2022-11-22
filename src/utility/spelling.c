#include <assert.h>

#include <clang-c/Index.h>
#include <ustring/str.h>

str_t* get_source_range_spelling(CXCursor cursor) {
    CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);
    CXSourceRange source_range = clang_getCursorExtent(cursor);

    CXToken* tokens = NULL;
    unsigned int tokens_num = 0;
    clang_tokenize(tu, source_range, &tokens, &tokens_num);

    str_t* source_range_spelling = str_new(NULL);

    for(unsigned int i = 0; i < tokens_num; i++) {
        CXString token_spelling = clang_getTokenSpelling(tu, tokens[i]);
        str_append(source_range_spelling, clang_getCString(token_spelling));
        str_append(source_range_spelling, " ");
        clang_disposeString(token_spelling);
    }

    clang_disposeTokens(tu, tokens, tokens_num);

    return source_range_spelling;
}
