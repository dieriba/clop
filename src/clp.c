#include "clp.h"
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

DResult clp_init(Clp *clp)
{
    if (clp == NULL)
        return D_ERR_INVALID_ARG;
    DResult result;
    if ((result = d_unordered_map_init_not_owned_usize_key(&clp->full_opt, sizeof(InternalOpt), DEFAULT_OPT, NULL)) != D_OK || (result = d_hash_set_init_not_owned_d_dyn_string_key(&clp->short_opt, DEFAULT_OPT, NULL)) != D_OK)
        return result;
    return D_OK;
}

static DResult try_new_internal_opt_from_opt_data(Clp *clp, InternalOpt *opt, const char *full_opt_name, const char *short_opt_name, const char *opt_description)
{

    opt->arg_opt = NULL;
    return D_OK;
}

DResult clp_register_opt(Clp *clp, usize option_nb, const char *full_opt_name, const char *short_opt_name, const char *opt_description)
{
    if (clp == NULL)
        return D_ERR_INVALID_ARG;

    return D_OK;
}

DResult clp_parse_args(Clp *clp, char **argv)
{
    if (clp == NULL || argv == NULL || *argv == NULL)
        return D_ERR_INVALID_ARG;

    return D_OK;
}

DResult clp_print_help(Clp *clp)
{
    if (clp == NULL)
        return D_ERR_INVALID_ARG;
    return D_OK;
}

DResult clp_get_opt_args(Clp *clp, usize option_nb)
{
    if (clp == NULL)
        return D_ERR_INVALID_ARG;

    return D_OK;
}

DResult clp_destroy(Clp *clp)
{
    if (clp == NULL)
        return D_ERR_INVALID_ARG;
    d_unordered_map_destroy(&clp->full_opt);
    d_hash_set_destroy(&clp->short_opt);
    return D_OK;
}