#ifndef __CFG_OPERATION_H__
#define __CFG_OPERATION_H__

#include <ustring/str.h>
#include <ustring/str_list.h>

struct operation {
    str_list_t* operands;
};

struct operation* operation_new();

void operation_drop(struct operation** self);

void operation_push_operand(struct operation* self, str_t* operand);

str_t* operation_to_str(const struct operation* self);

#endif /* __CFG_OPERATION_H__ */
