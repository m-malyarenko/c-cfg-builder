#include <stdlib.h>

#include <ustring/str.h>
#include <ustring/str_list.h>

#include "operation.h"

struct operation* operation_new() {
    struct operation* new_operation = malloc(sizeof(struct operation));

    new_operation->operands = str_list_new();
    return new_operation;
}

void operation_drop(struct operation** self) {
    if ((self == NULL) || (*self == NULL)) {
        return;
    }

    str_list_drop(&(*self)->operands);
    free(*self);
    *self = NULL;
}

void operation_push_operand(struct operation* self, str_t* operand) {
    if (self == NULL) {
        return;
    }

    str_list_push(self->operands, operand);
}

str_t* operation_to_str(const struct operation* self) {
    return (self == NULL) ? str_new(NULL) : str_list_join(self->operands, " ");
}
