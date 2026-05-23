#include <ctype.h>
#include "clp.h"
#include "d_hash_set.h"
#include "d_general_lib.h"

#define NO_DESC "no associated description"
#define START_OPT_CHAR '-'
#define ALPHABET_LOWERCASE "abcdefghijklmnopqrstuvwxyz"
#define ALPHABET_UPPERCASE "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define NUMERIC "0123456789"
#define HYPHEN "-"
#define DOUBLE_HYPHEN HYPHEN HYPHEN
#define UNDERSCORE "_"
#define clp_eprint(...) eprint(clp, __VA_ARGS__)
#define clp_eprint_exit(...) eprint_exit(clp, __VA_ARGS__)
#define FLAG_SHORT_OPT_NOT_SET 0xFF
#define STR_STARTS_WITH_HYPEN(s) *(s) == '-'
#define EQUAL '='

typedef void (*ConversionFn)(char *to_convert, Value *value);

void free_command(void *command)
{
    if (command == NULL)
        return;
    Command *cmd = command;
    d_dyn_array_destroy(&cmd->operands);
    d_dyn_array_destroy(&cmd->sub_commands);

    DDynArray *opts = &cmd->options;
    for (usize i = 0; i < opts->array.size; i++)
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

Option *clp_get_option_by_short(Command *command, char shrt)
{
    if (command == NULL)
        return NULL;
    return get_option_by_predicate(command, &shrt, eq_short_opt);
}

static inline bool eq_long_opt(Option *opt, void *ctx)
{
    return d_string_view_compare(opt->long_name, *((DStringView *)ctx));
}

Option *clp_get_option_by_long(Command *command, DStringView lng)
{
    if (command == NULL || lng.size == 0)
        return NULL;
    return get_option_by_predicate(command, &lng, eq_long_opt);
}

Operand *clp_get_operand(Command *command, DStringView operand_name)
{
    if (command == NULL)
        return NULL;
    DDynArray *operands = &command->operands;
    usize size = d_dyn_array_get_size_safe(operands);
    for (usize i = 0; i < size; i++)
    {
        Operand *operand = d_dyn_array_get_elem_addr_at_safe(operands, i);
        if (d_string_view_compare(operand->name, operand_name))
            return operand;
    }
    return NULL;
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
    if (clp_get_option_by_long(command, command_option->long_name))
        clp_eprint_exit("%s command option '--%s' already registered\n", command->name.data, command_option->long_name);
    if (clp_get_option_by_short(command, command_option->short_name))
        clp_eprint_exit("%s command option '-%c' already registered\n", command->name.data, command_option->short_name);
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
    DDynArray *ops = &command->operands;
    usize size = d_dyn_array_get_size_safe(ops);
    Operand *last_operand = size == 0 ? NULL : d_dyn_array_get_elem_addr_at_safe(ops, size - 1);

    if (get_command_operand_by_name(command, command_operand->name))
        clp_eprint("warning: operand name '%s' already used, consider a more descriptive name\n", command->name.data);
    else if (last_operand != NULL && (last_operand->required == false && command_operand->required == true))
        clp_eprint_exit("%s command required operand '%s' cannot follow optional operand '%s'\n", command->name.data, command_operand->name.data, last_operand->name.data);
    return d_dyn_array_push_back(&command->operands, command_operand);
}

static bool is_valid_short(char shrt)
{
    return isalnum(shrt);
}

static bool is_valid_long(DStringView lng)
{
    if (lng.size == 0 || lng.data[0] == HYPHEN || lng.data[0] == UNDERSCORE || d_string_view_find_first_char_not_in_set_from_start(lng, D_STRING_VIEW_FROM_LITERAL(ALPHABET_LOWERCASE ALPHABET_UPPERCASE HYPHEN UNDERSCORE NUMERIC)) != MAX_SIZE_T_VALUE)
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

static void exit_if_not_valid_long_opt_name(DStringView long_opt)
{
    if (is_valid_long(long_opt) == false)
    {
        clp_eprint("'--%s' is not a valid option name\n", long_opt.data);
        clp_eprint_exit("a long option must start with a letter and contain only [a-z A-Z 0-9 -]\n");
    }
}

static void exit_if_not_valid_short_opt_name(char short_opt)
{
    if (is_valid_short(short_opt) == false)
    {
        clp_eprint("'-%c' is not a valid option name\n", short_opt);
        clp_eprint_exit("a short option must be a single alphanumeric character [a-z A-Z 0-9]\n");
    }
}

DResult clp_init_option_raw(Option *opt, char *long_name, char *short_name, char *description, bool has_default_value,
                            OptAction action, Value value, Type type, bool required, bool global)
{
    if (opt == NULL || (long_name == NULL && short_name == NULL))
        return D_ERR_INVALID_ARG;
    char shrt_name = FLAG_SHORT_OPT_NOT_SET;
    opt->long_name = d_string_view_from_c_string(long_name);

    if (short_name)
        exit_if_not_valid_shot_opt_name(shrt_name = *short_name);
    if (long_name)
        exit_if_not_valid_long_opt_name(opt->long_name);

    if (action == ARG_ACT_COUNT && type != TYPE_USIZE)
    {
        if (long_name)
            clp_eprint("option '--%s' has action 'count' but type '%s' is not valid\n", long_name, type_to_str(type));
        else
            clp_eprint("option '-%c' has action 'count' but type '%s' is not valid\n", shrt_name, type_to_str(type));
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

static void s_to_usize(char *s, Value *value)
{
}

static void s_to_double(char *s, Value *value)
{
}

static void s_to_long(char *s, Value *value)
{
}

static void s_to_char(char *s, Value *value)
{
}

static void s_to_string(char *s, Value *value)
{
}

static void s_to_bool(char *s, Value *value)
{
}

static void set_bool(char *s, Value *value)
{
    (void)s;
    value->value_bool = true;
}

static ConversionFn type_to_conversion_fn(Type type)
{
    switch (type)
    {
    case TYPE_USIZE:
        return s_to_usize;
    case TYPE_LONG:
        return s_to_long;
    case TYPE_STR:
        return s_to_string;
    case TYPE_CHAR:
        return s_to_char;
    case TYPE_DOUBLE:
        return s_to_double;
    case TYPE_BOOL:
        return s_to_bool;
    default:
        break;
    }
    return NULL;
}

static void set_opt_value(Command *root, Option *opt, char *operand, char *prefix, char *opt_name, usize len_name)
{
    ConversionFn conversion_fn = type_to_conversion_fn(opt->type);

    switch (opt->action)
    {
    case ARG_ACT_SET:
        if (opt->type != TYPE_BOOL)
        {
            if (operand == NULL)
                clp_eprint_exit("%s command '%s%.*s' require a value", root->name, prefix, (int)len_name, opt_name);
            conversion_fn(operand, &opt->value);
        }
        else
            opt->value.value_bool = true;
        break;
    case ARG_ACT_COUNT:
        opt->value.value_usize++;
        break;
    case ARG_ACT_LIST:
        DStringView list = d_string_view_from_c_string(operand);
        usize i = 0, j = 0;
        while (1)
        {
            j = d_string_view_find_first_matching_char_from_index(list, ',', i);
            DStringView sub = d_string_view_subview(list, i, j - i);
            DResult result;
            if ((result = d_dyn_array_push_back(&opt->value.value_list, &sub)) != D_OK)
                clp_eprint("internal error: %s", d_error_print_result_as_str(result));
            if (j == MAX_SIZE_T_VALUE)
                break;
            i = j + 1;
        }
        break;
    default:
        break;
    }
    if (opt->value_set == false)
        opt->value_set = true;
}

static char **set_operand_value(Command *root, usize cursor, char *raw_operand, char **argv, bool consume_all)
{
    DDynArray *operands = &root->operands;
    usize operands_size = d_dyn_array_get_size_safe(operands);

    if (cursor >= operands_size && raw_operand != NULL)
        clp_eprint_exit("%s too many operands provided\n", root->name.data);
    else if (raw_operand == NULL)
        return argv;

    Operand *operand = d_dyn_array_get_elem_addr_at_safe(operands, cursor);
    ConversionFn conversion_fn = type_to_conversion_fn(operand->type);

    switch (operand->action)
    {
    case OPERAND_ACT_SET_UNIQUE:
        conversion_fn(raw_operand, &operand->value);
        argv++;
        break;
    case OPERAND_ACT_LIST:
        while (1)
        {
            DStringView list = d_string_view_from_c_string(raw_operand);
            DResult result;
            if ((result = d_dyn_array_push_back(&operand->value.value_list, &list)) != D_OK)
                clp_eprint("internal error: %s", d_error_print_result_as_str(result));
            raw_operand = *(++argv);
            if (raw_operand == NULL || (!consume_all && STR_STARTS_WITH_HYPEN(raw_operand)))
                break;
        }
        break;
    default:
        break;
    }
    if (operand->value_set == false)
        operand->value_set = true;
    return argv;
}

static inline void exit_if_invalid_opt(Command *root, Option *opt, char *prefix, char *name, usize len)
{
    if (opt == NULL)
        clp_eprint_exit("%s command unknown option '%s%.*s'\n", root->name, prefix, (int)len, name);
}

static char **parse_remaining_operands(Command *root, usize *operand_cursor, char **argv)
{
    usize cursor = *operand_cursor;
    while (1)
    {
        argv = set_operand_value(root, cursor, *argv, argv, true);
        if (*argv == NULL)
            break;
        cursor++;
    }
    *operand_cursor = cursor;
    return argv;
}

static char **parse_long_opt(Command *root, char *lng_opt, char **argv)
{
    DStringView long_opt = d_string_view_from_c_string(lng_opt);
    usize eq_pos = d_string_view_find_first_matching_char_from_start(long_opt, '=');
    bool has_inline_value = eq_pos != MAX_SIZE_T_VALUE;
    if (has_inline_value)
        long_opt = d_string_view_subview(long_opt, 0, eq_pos);
    exit_if_not_valid_long_opt_name(long_opt);
    Option *opt = clp_get_option_by_long(root, long_opt);
    exit_if_invalid_opt(root, opt, DOUBLE_HYPHEN, long_opt.data, long_opt.size);
    char *operand = has_inline_value ? &long_opt.data[eq_pos + 1] : *argv;
    set_opt_value(root, opt, operand, DOUBLE_HYPHEN, long_opt.data, long_opt.size);
    return argv + (!has_inline_value && opt->type != TYPE_BOOL);
}

static char **parse_short_opts(Command *root, char *short_opt, char **argv)
{
    usize i = 0;
    do
    {
        Option *opt = clp_get_option_by_short(root, short_opt[i]);
        exit_if_invalid_opt(root, opt, HYPHEN, &short_opt[i], 1);
        if (opt->type != TYPE_BOOL)
        {
            bool consume_next_argv = short_opt[i + 1] == 0;
            char *operand = consume_next_argv == true ? *argv : &short_opt[i + 1];
            set_opt_value(root, opt, operand, HYPHEN, &short_opt[i], 1);
            return argv + (consume_next_argv == true);
        }
        set_opt_value(root, opt, NULL, HYPHEN, &short_opt[i], 1);
    } while (short_opt[++i] != 0);

    return argv;
}

static char **interpret_hyphen(Command *root, char *arg, char **argv, usize *operand_cusor)
{
    ++argv;
    if (STR_STARTS_WITH_HYPEN(arg))
    {
        if (arg[1] == 0)
            argv = parse_remaining_operands(root, operand_cusor, argv);
        else
            argv = parse_long_opt(root, &arg[1], argv);
    }
    else
        argv = parse_short_opts(root, arg, argv);
    return argv;
}

static void exit_if_command_required_nb_opts_not_met(Command *root)
{
    DDynArray *opts = &root->options;
    usize size = d_dyn_array_get_size_safe(opts);
    bool required_opts_not_set = false;
    for (size_t i = 0; i < size; i++)
    {
        Option *opt = d_dyn_array_get_elem_addr_at_safe(opts, i);
        if (opt->required == true && opt->value_set == false)
        {
            if (required_opts_not_set == false)
            {
                clp_eprint("command %s: the following options were not provided:\n", root->name.data);
                required_opts_not_set = true;
            }
            if (opt->long_name.size != 0)
                clp_eprint("--%s\n", opt->long_name.data);
            else
                clp_eprint("-%c\n", opt->short_name);
        }
    }
    if (required_opts_not_set)
        exit(EXIT_FAILURE);
}

static void exit_if_command_required_nb_operands_size_not_met(Command *root)
{
    DDynArray *operands = &root->operands;
    usize size = d_dyn_array_get_size_safe(operands);
    bool required_operands_not_set = false;
    for (size_t i = 0; i < size; i++)
    {
        Operand *operand = d_dyn_array_get_elem_addr_at_safe(operands, i);
        if (operand->required == true && operand->value_set == false)
        {
            if (required_operands_not_set == false)
            {
                clp_eprint("command %s: the following operands were not provided:\n", root->name.data);
                required_operands_not_set = true;
            }
            clp_eprint("<%s>\n", operand->name.data);
        }
    }
    if (required_operands_not_set)
        exit(EXIT_FAILURE);
}

static void parse(Command *root, char **argv, Command **command)
{
    ++argv; // skip program name at first call then skip current command name already parsed
    char *s;
    DDynArray *sub_commands = &root->sub_commands;
    usize sub_commands_size = d_dyn_array_get_size_safe(sub_commands);
    usize cmd_parsed_operand = 0;
    bool operand_mode = false;

    if (*argv == NULL)
    {
        // print command usage/help
    }

    while ((s = *argv) != NULL)
    {
        if (STR_STARTS_WITH_HYPEN(s))
        {
            argv = interpret_hyphen(root, &s[1], argv, &cmd_parsed_operand);
            continue;
        }

        if (operand_mode == false)
        {
            for (usize i = 0; i < sub_commands_size; i++)
            {
                Command *sub_cmd = d_dyn_array_get_elem_addr_at_safe(sub_commands, i);
                if (d_string_view_compare_against_c_string(sub_cmd->name, s))
                {
                    *command = sub_cmd;
                    parse(sub_cmd, argv, command);
                    return;
                }
            }
        }

        if (!operand_mode)
            operand_mode = true;
        argv = set_operand_value(root, cmd_parsed_operand++, s, argv, false);
    }

    exit_if_command_required_nb_operands_size_not_met(root);
    exit_if_command_required_nb_opts_not_met(root);
}

DResult clp_parse_args(Command *root, char **argv, Command **command)
{
    if (root == NULL || argv == NULL || *argv == NULL)
        return D_ERR_INVALID_ARG;
    parse(root, argv, command);
    return D_OK;
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