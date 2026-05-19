#include <ctype.h>
#include "clp.h"
#include "d_hash_set.h"

#define NO_DESC "no associated description"
#define START_OPT_CHAR '-'
#define ALPHABET_LOWERCASE "abcdefghijklmnopqrstuvwxyz"
#define ALPHABET_UPPERCASE "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define NUMERIC "0123456789"
#define HYPHEN "-"
#define UNDERSCORE "_"

void free_command(void *command)
{
    if (command == NULL)
        return;
    Command *cmd = command;
    d_dyn_array_destroy(&cmd->operands);
    d_dyn_array_destroy(&cmd->sub_commands);
    d_dyn_array_destroy(&cmd->options);
    d_dyn_array_destroy(&cmd->extra);
}

DResult clp_init_command(Command *command, char *name, char *description, int code)
{
    if (command == NULL || name == NULL)
        return D_ERR_INVALID_ARG;
    command->name = d_string_view_from_c_string(name);
    command->description = description == NULL ? NO_DESC : description;
    command->code = code;
    command->parent_command = NULL;
    DResult op_result;

    if ((op_result = d_dyn_array_default_init(&command->sub_commands, Command, free_command, RAW_BUF_OPT_NONE)) != D_OK || (op_result = d_dyn_array_default_init(&command->options, Option, NULL, RAW_BUF_OPT_NONE)) != D_OK ||
        (op_result = d_dyn_array_default_init(&command->extra, char *, NULL, RAW_BUF_OPT_NONE)) != D_OK || (op_result = d_dyn_array_default_init(&command->operands, Operand, NULL, RAW_BUF_OPT_NONE)) != D_OK)
        return op_result;
    return D_OK;
}

DResult clp_add_command_sub_command(Command *command, Command *sub_command)
{
    if (command == NULL || sub_command == NULL)
        return D_ERR_INVALID_ARG;
    sub_command->parent_command = command;
    return d_dyn_array_push_back(&command->sub_commands, sub_command);
}

static inline bool eq_short_opt(Option *opt, void *ctx)
{
    return opt->short_name == *(char *)ctx;
}

static Option *get_option_by_short(Command *command, char shrt)
{
    return get_option_by_predicate(command, &shrt, eq_short_opt);
}

static inline bool eq_long_opt(Option *opt, void *ctx)
{
    return d_string_view_compare(opt->long_name, *((DStringView *)ctx));
}

static Option *get_option_by_long(Command *command, DStringView lng)
{
    return get_option_by_predicate(command, &lng, eq_long_opt);
}

static Option *get_option_by_predicate(Command *command, void *ctx, bool (*predicate)(Option *, void *))
{
    DDynArray *opts = &command->options;
    usize size = d_dyn_array_get_size_safe(opts);
    for (usize i = 0; i < size; i++)
    {
        Option *option = d_dyn_array_get_elem_addr_at_safe(opts, i);
        if (predicate(option, ctx))
            return option;
    }
    return NULL;
}

DResult clp_add_command_option(Command *command, Option *command_option, DError *error)
{
    if (command == NULL || command_option == NULL)
        return D_ERR_INVALID_ARG;
    if (get_option_by_long(command, command_option->long_name))
    {
        //
    }
    else if (get_option_by_short(command, command_option->short_name))
    {
        //
    }
    return d_dyn_array_push_back(&command->options, command_option);
}

static Operand *get_command_operand_by_name(Command *command, DStringView name)
{
    DDynArray *operands = &command->operands;
    usize size = d_dyn_array_get_size_safe(operands);
    for (usize i = 0; i < size; i++)
    {
        Operand *operand = d_dyn_array_get_elem_addr_at_safe(operands, i);
        if (d_string_view_compare(operand->name, name))
            return operand;
    }
    return NULL;
}

DResult clp_add_command_operand(Command *command, Operand *command_operand, DError *error)
{
    if (command == NULL || command_operand == NULL)
        return D_ERR_INVALID_ARG;
    if (get_command_operand_by_name(command, command_operand->name))
    {
        //
    }
    return d_dyn_array_push_back(&command->operands, command_operand);
}

void construct_error_command(Command *cmd, DError *error)
{
}

static bool is_valid_short(char shrt)
{
    return isalnum(shrt);
}

static bool is_valid_long(DStringView lng)
{
    if (lng.size == 0 || lng.data[0] == HYPHEN || lng.data[0] == UNDERSCORE || d_string_view_find_first_char_not_in_set_from_start(lng, D_STRING_VIEW_FROM_LITERAL(ALPHABET_LOWERCASE ALPHABET_UPPERCASE HYPHEN UNDERSCORE)) != MAX_SIZE_T_VALUE)
        return false;
    return true;
}

static bool action_valid_for_type(ArgAction arg_action, Type type, DError *error)
{
    switch (arg_action)
    {
    case ARG_ACT_COUNT:
        if (type != OPT_TYPE_USIZE)
            return false;
    default:
        break;
    }
    return true;
}

DResult clp_init_option_raw(Option *opt, char *long_name, char short_name, char *description, bool has_default_value,
                            ArgAction action, Value value, Type type, bool required, bool global, DError *error)
{
    if (opt == NULL || long_name == NULL)
        return D_ERR_INVALID_ARG;

    if (is_valid_short(short_name) == false)
        return D_ERR;
    else if (is_valid_long(opt->long_name = d_string_view_from_c_string(long_name)) == false)
        return D_ERR;

    if (action_valid_for_type(action, type, error) == false)
        return D_ERR;

    opt->value_set = false;
    opt->action = action;
    opt->short_name = short_name;
    opt->description = description == NULL ? NO_DESC : description;
    opt->has_default_value = has_default_value;
    opt->value = value;
    opt->type = type;
    opt->required = required;
    opt->global = global;
    return D_OK;
}

static inline bool is_valid_operand(DStringView name)
{
    return is_valid_long(name);
}

DResult clp_init_operand_raw(Operand *operands, char *name, char *description, bool has_default_value, OperanAction action, Value value, Type type, bool required, DError *error)
{
    if (operands == NULL || name == NULL)
        return D_ERR_INVALID_ARG;

    if (is_valid_operand(operands->name = d_string_view_from_c_string(name)) == false)
        return D_ERR;

    operands->value_set = false;
    operands->action = action;
    operands->description = description == NULL ? NO_DESC : description;
    operands->has_default_value = has_default_value;
    operands->value = value;
    operands->type = type;
    operands->required = required;
    return D_OK;
}

static DResult parse_short_opt(Command *root, char shrt)
{
}

static DResult parse_long_opt(Command *root, char *lng)
{
}

static DResult parse_option(Command *root, char *opt)
{
    DResult op_result;
    if (opt[1] == 0)
    {
        op_result = parse_short_opt(root, *opt);
    }
    else if (opt[1] == '-')
    {
        if (opt[2] == 0)
        {
        }
        else
        {
            op_result = parse_long_opt(root, opt);
        }
    }
    else
    {
        // d_error_new();
    }
}

static DResult parse(Command *root, char **argv, Command **command, DError *error)
{
    ++argv; // skip program name at first call then skip current command name already parsed
    char *s;
    DDynArray *sub_commands = &root->sub_commands;
    usize size = d_dyn_array_get_size_safe(sub_commands);
    while ((s = *argv) != NULL)
    {
        if (s[0] == HYPHEN)
        {
            
        }

        for (usize i = 0; i < size; i++)
        {
            Command *sub_cmd = d_dyn_array_get_elem_addr_at_safe(sub_commands, i);
            if (d_string_view_compare_against_c_string(sub_cmd->name, s))
            {
                *command = sub_cmd;
                DResult op_result = parse(sub_cmd, argv, command, error);
                if (op_result != D_OK)
                    return op_result;
            }
        }
        argv++;
    }

    return D_OK;
}

DResult clp_parse_args(Command *root, char **argv, Command **command, DError *error)
{
    if (root == NULL || argv == NULL || *argv == NULL)
        return D_ERR_INVALID_ARG;
    return parse(root, argv, command, error);
}

void clp_cleanup(Command *root)
{
    if (root == NULL)
        return;
    DDynArray *sub_commands = &root->sub_commands;
    usize size = d_dyn_array_get_size_safe(sub_commands);
    for (usize i = 0; i < size; i++)
    {
        clp_cleanup(d_dyn_array_get_elem_addr_at_safe(sub_commands, i));
    }
    free_command(root);
}