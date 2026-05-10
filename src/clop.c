#include "clop.h"
#include "d_unordered_map.h"
#include "d_hash_set.h"
#include "d_string_view.h"

#define OPTION_START "-"
#define DEFAULT_OPT 0x8

typedef struct InternalOpt
{
    char *full_opt_name;
    char *short_opt_name;
    char *opt_desc;
    char **arg_opt;
} InternalOpt;

DResult clop_init(Clop *clop)
{
    if (clop == NULL)
        return D_ERR_INVALID_ARG;
    DResult result;
    if ((result = d_unordered_map_init_not_owned_usize_key(&clop->full_opt, sizeof(InternalOpt), DEFAULT_OPT, NULL)) != D_OK || (result = d_hash_set_init_not_owned_d_dyn_string_key(&clop->short_opt, DEFAULT_OPT, NULL)) != D_OK)
        return result;
    return D_OK;
}

static DResult try_new_internal_opt_from_opt_data(Clop *clop, InternalOpt *opt, const char *full_opt_name, const char *short_opt_name, const char *opt_description)
{

    opt->arg_opt = NULL;
    return D_OK;
}

DResult clop_register_opt(Clop *clop, usize option_nb, const char *full_opt_name, const char *short_opt_name, const char *opt_description)
{
    if (clop == NULL)
        return D_ERR_INVALID_ARG;

    return D_OK;
}

DResult clop_parse_args(Clop *clop, char **argv)
{
    if (clop == NULL || argv == NULL || *argv == NULL)
        return D_ERR_INVALID_ARG;


    

    return D_OK;
}

DResult clop_print_help(Clop *clop)
{
    if (clop == NULL)
        return D_ERR_INVALID_ARG;
    return D_OK;
}

DResult clop_get_opt_args(Clop *clop, usize option_nb)
{
    if (clop == NULL)
        return D_ERR_INVALID_ARG;

    return D_OK;
}

DResult clop_destroy(Clop *clop)
{
    if (clop == NULL)
        return D_ERR_INVALID_ARG;
    d_unordered_map_destroy(&clop->full_opt);
    d_hash_set_destroy(&clop->short_opt);
    return D_OK;
}