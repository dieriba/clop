#ifndef CLOP_H
#define CLOP_H

#include "d_types.h"

typedef struct Clp
{
    DUnorderedMap full_opt;
    DHashSet short_opt;
} Clp;

DResult clp_init(Clp *clp);
DResult clp_register_opt(Clp *clp, usize option_nb, char *full_opt_name, char *short_opt_name, char *opt_description);
DResult clp_parse_args(Clp *clp, char **argv);
DResult clp_print_help(Clp *clp);
DResult clp_get_opt_args(Clp *clp, usize option_nb);
DResult clp_destroy(Clp *clp);
#endif