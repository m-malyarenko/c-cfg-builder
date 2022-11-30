#include <stdio.h>

#include <ustring/str.h>

#include "../builder/cfg_builder.h"
#include "dot_converter.h"

str_t* convert_to_dot(struct cfg_builder_output cfg) {
    #ifndef NDEBUG
    for (size_t i = 0; i < cfg.cfg_nodes->len; i++) {
        struct basic_block* bb = cfg.cfg_nodes->buffer[i];
        printf("Node #%lu: %p\n", i + 1, (void*) bb);

        for (size_t j = 0; j < bb->operations.len; j++) {
            str_t* op = str_list_join(bb->operations.buffer[j], "");
            printf("\t%lu %s\n", j + 1, str_as_ptr(op));
            str_drop(&op);
        }

        printf("\t---\n");

        for (size_t j = 0; j < bb->links.len; j++) {
            printf("\tLink #%lu: %p\n", j + 1, (void*) bb->links.buffer[j]);
        }

        printf("\n\n");
    }
    #endif

    str_t* dot_buffer = str_new(NULL);

    str_append(dot_buffer, "digraph cfg {\n");
    str_append(dot_buffer, "\tnode [shape = record]\n\n");

    char sprintf_buffer[64];

    for (size_t i = 0; i < cfg.cfg_nodes->len; i++) {
        struct basic_block* bb = cfg.cfg_nodes->buffer[i];
        if (bb == cfg.cfg_sink) {
            sprintf(sprintf_buffer, "\tnode_%p[shape=doublecircle label=\"", (void*) bb);
        } else {
            sprintf(sprintf_buffer, "\tnode_%p[label=\"", (void*) bb);
        }
        str_append(dot_buffer, sprintf_buffer);

        for (size_t j = 0; j < bb->operations.len; j++) {
            str_t* op = str_list_join(bb->operations.buffer[j], "");
            str_replace(op, ">", "\\>");
            str_replace(op, "<", "\\<");
            str_replace(op, "|", "\\|");
            str_replace(op, "\"", "\\\"");
            str_append(dot_buffer, str_as_ptr(op));
            str_append(dot_buffer, "\\n");
            str_drop(&op);
        }
        str_append(dot_buffer, "\"];\n");
    }

    for (size_t i = 0; i < cfg.cfg_nodes->len; i++) {
        struct basic_block* bb = cfg.cfg_nodes->buffer[i];

        for (size_t j = 0; j < bb->links.len; j++) {
            struct basic_block* link_bb = bb->links.buffer[j];
            if (link_bb != NULL) {
                sprintf(sprintf_buffer, "\tnode_%p -> node_%p[color=\"", (void*) bb, (void*) link_bb);
                str_append(dot_buffer, sprintf_buffer);
                if (bb->links.len != 2) {
                    str_append(dot_buffer, "black");
                } else {
                    str_append(dot_buffer, (j == 0) ? "green" : "red");
                }
                str_append(dot_buffer, "\"];\n");
            }
        }
    }

    str_append(dot_buffer, "\n}\n");

    return dot_buffer;
}
