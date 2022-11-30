#ifndef __UTILITY_SPELLING_H__
#define __UTILITY_SPELLING_H__

#include <clang-c/Index.h>
#include <ustring/str.h>

str_t* get_cursor_range_spelling(CXCursor cursor);

#endif /* __UTILITY_SPELLING_H__ */
