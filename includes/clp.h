#ifndef CLOP_H
#define CLOP_H

#include "d_types.h"

typedef struct Clop
{
    DUnorderedMap full_opt;
    DHashSet short_opt;
} Clop;

DResult clop_init(Clop *clop);
DResult clop_register_opt(Clop *clop, usize option_nb, char *full_opt_name, char *short_opt_name, char *opt_description);
DResult clop_parse_args(Clop *clop, char **argv);
DResult clop_print_help(Clop *clop);
DResult clop_get_opt_args(Clop *clop, usize option_nb);
DResult clop_destroy(Clop *clop);
#endif