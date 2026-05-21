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
#define clp_eprint(...) eprint(clp, __VA_ARGS__)
#define clp_eprint_exit(...) eprint_exit(clp, __VA_ARGS__)
#define FLAG_SHORT_OPT_NOT_SET 0xFF

void free_command(void *command)
{
    if (command == NULL)
        return;
    Command *cmd = command;
    d_dyn_array_destroy(&cmd->operands);
    d_dyn_array_destroy(&cmd->sub_commands);

    DDynArray *opts = &cmd->options;
    for (size_t i = 0; i < opts->array.size; i++)
    {
        Option *opt = d_dyn_array_get_elem_addr_at_safe(opts, i);
        if (opt->type == TYPE_KV)
            d_unordered_map_destroy(&opt->value.value_kv);
        else if (opt->action == ARG_ACT_LIST)
            d_dyn_array_destroy(&opt->value.value_list);
    }

    d_dyn_array_destroy(&cmd->options);
    d_dyn_array_destroy(&cmd->extra);
}

DResult clp_init_command(Command *command, int code, char *name, char *description)
{
    if (command == NULL || name == NULL)
        return D_ERR_INVALID_ARG;
    command->name = d_string_view_from_c_string(name);
    command->description = description == NULL ? NO_DESC : description;
    command->code = code;
    command->parent_command = NULL;
    DResult op_result;

    if ((op_result = d_dyn_array_default_init(&command->sub_commands, Command, free_command, NULL, RAW_BUF_OPT_NONE)) != D_OK || (op_result = d_dyn_array_default_init(&command->options, Option, NULL, NULL, RAW_BUF_OPT_NONE)) != D_OK ||
        (op_result = d_dyn_array_default_init(&command->extra, char *, NULL, NULL, RAW_BUF_OPT_NONE)) != D_OK || (op_result = d_dyn_array_default_init(&command->operands, Operand, NULL, NULL, RAW_BUF_OPT_NONE)) != D_OK)
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

Option *get_option_by_short(Command *command, char shrt)
{
    return get_option_by_predicate(command, &shrt, eq_short_opt);
}

static inline bool eq_long_opt(Option *opt, void *ctx)
{
    return d_string_view_compare(opt->long_name, *((DStringView *)ctx));
}

Option *get_option_by_long(Command *command, DStringView lng)
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

DResult clp_add_command_option(Command *command, Option *command_option)
{
    if (command == NULL || command_option == NULL)
        return D_ERR_INVALID_ARG;
    if (get_option_by_long(command, command_option->long_name))
        clp_eprint_exit("%s internal error: option '--%s' already registered\n", command->name.data, command_option->long_name);
    if (get_option_by_short(command, command_option->short_name))
        clp_eprint_exit("%s internal error: option '-%c' already registered\n", command->name.data, command_option->short_name);
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

DResult clp_add_command_operand(Command *command, Operand *command_operand)
{
    if (command == NULL || command_operand == NULL)
        return D_ERR_INVALID_ARG;
    if (get_command_operand_by_name(command, command_operand->name))
        clp_eprint("warning: operand name '%s' already used, consider a more descriptive name\n", command->name.data);
    return d_dyn_array_push_back(&command->operands, command_operand);
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

static const char *type_to_str(Type type)
{
    switch (type)
    {
    case TYPE_LONG:
        return "long";
    case TYPE_BOOL:
        return "bool";
    case TYPE_USIZE:
        return "usize";
    case TYPE_STR:
        return "str";
    case TYPE_CHAR:
        return "char";
    case TYPE_DOUBLE:
        return "double";
    default:
        break;
    }
    return "UNKNOWN TYPE";
}

DResult clp_init_option_raw(Option *opt, char *long_name, char *short_name, char *description, bool has_default_value,
                            OptAction action, Value value, Type type, bool required, bool global)
{
    if (opt == NULL || (long_name == NULL && short_name == NULL))
        return D_ERR_INVALID_ARG;
    char shrt_name = FLAG_SHORT_OPT_NOT_SET;
    if (short_name && is_valid_short(shrt_name = *short_name) == false)
    {
        clp_eprint("internal error: '-%c' is not a valid option name\n", shrt_name);
        clp_eprint_exit("a short option must be a single alphanumeric character [a-z A-Z 0-9]\n");
    }
    else if (is_valid_long(opt->long_name = d_string_view_from_c_string(long_name)) == false)
    {
        clp_eprint("internal error: '--%s' is not a valid option name\n", long_name);
        clp_eprint_exit("a long option must start with a letter and contain only [a-z A-Z 0-9 -]\n");
    }

    if (action == ARG_ACT_COUNT && type != TYPE_USIZE)
    {
        if (long_name)
            clp_eprint("internal error: option '--%s' has action 'count' but type '%s' is not valid\n", long_name, type_to_str(type));
        else
            clp_eprint("internal error: option '-%c' has action 'count' but type '%s' is not valid\n", shrt_name, type_to_str(type));
        clp_eprint_exit("count action requires an integer type [u8 u16 u32 u64]\n");
    }

    opt->value_set = false;
    opt->action = action;
    opt->short_name = shrt_name;
    opt->description = description == NULL ? NO_DESC : description;
    opt->has_default_value = has_default_value;
    opt->value = value;
    opt->type = type;
    opt->required = required;
    opt->global = global;
    return D_OK;
}

DResult clp_init_operand_raw(Operand *operands, char *name, char *description, bool has_default_value, OperanAction action, Value value, Type type, bool required)
{
    if (operands == NULL || name == NULL)
        return D_ERR_INVALID_ARG;

    operands->value_set = false;
    operands->action = action;
    operands->description = description == NULL ? NO_DESC : description;
    operands->has_default_value = has_default_value;
    operands->value = value;
    operands->type = type;
    operands->required = required;
    return D_OK;
}

static DResult parse(Command *root, char **argv, Command **command)
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
                DResult op_result = parse(sub_cmd, argv, command);
                if (op_result != D_OK)
                    return op_result;
            }
        }
        argv++;
    }

    return D_OK;
}

DResult clp_parse_args(Command *root, char **argv, Command **command)
{
    if (root == NULL || argv == NULL || *argv == NULL)
        return D_ERR_INVALID_ARG;
    return parse(root, argv, command);
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