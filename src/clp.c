#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "clp.h"
#include "d_hash_set.h"
#include "d_general_lib.h"
#include "converter.h"

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
#define clp_invalid_arg_exit(...) clp_eprint_exit("invalid argument: " __VA_ARGS__)
#define STR_STARTS_WITH_HYPEN(s) *(s) == '-'
#define HELP_OPT "help"
#define HELP_COL_GAP 4
#define FLAG_SHORT_OPT_NOT_SET 0xFF

static void _free_command(void **command);

static bool is_valid_short(char shrt)
{
    return isalnum(shrt);
}

static bool is_valid_long(DStringView lng)
{
    if (lng.size == 0 || lng.data[0] == '-' || lng.data[0] == '_' || d_string_view_find_first_char_not_in_set_from_start(lng, D_STRING_VIEW_FROM_LITERAL(ALPHABET_LOWERCASE ALPHABET_UPPERCASE HYPHEN UNDERSCORE NUMERIC)) != MAX_SIZE_T_VALUE)
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

static void exit_if_not_valid_short_opt_name(char short_opt)
{
    if (is_valid_short(short_opt) == false)
    {
        clp_eprint("'-%c' is not a valid option name\n", short_opt);
        clp_eprint_exit("a short option must be a single alphanumeric character [a-z A-Z 0-9]\n");
    }
}

static void exit_if_not_valid_long_opt_name(DStringView long_opt)
{
    if (is_valid_long(long_opt) == false)
    {
        clp_eprint("'--%s' is not a valid option name\n", long_opt.data);
        clp_eprint_exit("a long option must start with a letter and contain only [a-z A-Z 0-9 -]\n");
    }
}

void clp_init_command(Command *command, int code, char *name, char *description)
{
    if (command == NULL || name == NULL)
        clp_invalid_arg_exit("null argument to clp_init_command\n");
    command->name = d_string_view_from_c_string(name);
    command->description = description == NULL ? NO_DESC : description;
    command->code = code;
    command->parent_command = NULL;

    if (d_dyn_array_default_ptr_arr_init(&command->sub_commands, (DestroyElemFn)_free_command, NULL, RAW_BUF_OPT_NONE) != D_OK ||
        d_dyn_array_default_ptr_arr_init(&command->options, NULL, NULL, RAW_BUF_OPT_NONE) != D_OK ||
        d_dyn_array_default_ptr_arr_init(&command->extra, NULL, NULL, RAW_BUF_OPT_NONE) != D_OK ||
        d_dyn_array_default_ptr_arr_init(&command->operands, NULL, NULL, RAW_BUF_OPT_NONE) != D_OK)
        clp_eprint_exit("out of memory\n");
}

void clp_init_option_raw(Option *opt, char *long_name, char *short_name, char *description, bool has_default_value,
                         OptAction action, Value value, Type type, bool required, bool global)
{
    if (opt == NULL || (long_name == NULL && short_name == NULL))
        clp_invalid_arg_exit("null argument to clp_init_option_raw\n");
    char shrt_name = FLAG_SHORT_OPT_NOT_SET;
    opt->long_name = d_string_view_from_c_string(long_name);

    if (short_name)
        exit_if_not_valid_short_opt_name(shrt_name = *short_name);
    if (long_name)
    {
        if (PSEUDO_FAST_STRCMP(long_name, HELP_OPT))
            clp_invalid_arg_exit("%s is a reserved option\n", long_name);
        exit_if_not_valid_long_opt_name(opt->long_name);
    }

    if (action == ARG_ACT_COUNT && type != TYPE_USIZE)
    {
        if (long_name)
            clp_eprint("option '--%s' has action 'count' but type '%s' is not valid\n", long_name, type_to_str(type));
        else
            clp_eprint("option '-%c' has action 'count' but type '%s' is not valid\n", shrt_name, type_to_str(type));
        clp_eprint_exit("count action requires an integer type [u8 u16 u32 u64]\n");
    }

    if (has_default_value == false && action == ARG_ACT_LIST)
    {
        if (d_dyn_array_init(&opt->value.value_list, sizeof(DStringView), 1, NULL, NULL, RAW_BUF_OPT_NONE) != D_OK)
            clp_eprint_exit("out of memory\n");
    }
    else
        opt->value = value;

    opt->value_set = false;
    opt->action = action;
    opt->short_name = shrt_name;
    opt->description = description == NULL ? NO_DESC : description;
    opt->has_default_value = has_default_value;
    opt->type = type;
    opt->required = required;
    opt->global = global;
    opt->has_args = !(type == TYPE_BOOL || action == ARG_ACT_COUNT);
}

void clp_init_operand_raw(Operand *operand, char *name, char *description, bool has_default_value, OperanAction action, Value value, Type type, bool required)
{
    if (operand == NULL || name == NULL || *name == 0)
        clp_invalid_arg_exit("null argument to clp_init_operand_raw\n");

    if (has_default_value == false && action == OPERAND_ACT_LIST)
    {
        if (d_dyn_array_init(&operand->value.value_list, sizeof(DStringView), 1, NULL, NULL, RAW_BUF_OPT_NONE) != D_OK)
            clp_eprint_exit("out of memory\n");
    }
    else
        operand->value = value;

    operand->value_set = false;
    operand->action = action;
    operand->description = description == NULL ? NO_DESC : description;
    operand->has_default_value = has_default_value;
    operand->type = type;
    operand->required = required;
    operand->name = d_string_view_from_c_string(name);
}

static inline bool eq_short_opt(Option *opt, void *ctx)
{
    return opt->short_name == *(char *)ctx;
}

static inline bool eq_long_opt(Option *opt, void *ctx)
{
    return d_string_view_compare(opt->long_name, *((DStringView *)ctx));
}

static Option *get_option_by_predicate(Command *command, void *ctx, bool (*predicate)(Option *, void *))
{
    DDynArray *opts = &command->options;
    usize size = d_dyn_array_get_size_safe(opts);
    for (usize i = 0; i < size; i++)
    {
        Option *option = d_dyn_array_get_elem_deref_addr_at_safe(opts, i);
        if (predicate(option, ctx) == true)
            return option;
    }
    return NULL;
}

static Operand *get_command_operand_by_name(Command *command, DStringView name)
{
    DDynArray *operands = &command->operands;
    usize size = d_dyn_array_get_size_safe(operands);
    for (usize i = 0; i < size; i++)
    {
        Operand *operand = d_dyn_array_get_elem_deref_addr_at_safe(operands, i);
        if (d_string_view_compare(operand->name, name))
            return operand;
    }
    return NULL;
}

Option *clp_get_option_by_short(Command *command, char shrt)
{
    if (command == NULL)
        return NULL;
    return get_option_by_predicate(command, &shrt, eq_short_opt);
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
        Operand *operand = d_dyn_array_get_elem_deref_addr_at_safe(operands, i);
        if (d_string_view_compare(operand->name, operand_name))
            return operand;
    }
    return NULL;
}

void clp_add_command_sub_command(Command *command, Command *sub_command)
{
    if (command == NULL || sub_command == NULL)
        clp_invalid_arg_exit("null argument to clp_add_command_sub_command\n");
    if (d_dyn_array_get_size_safe(&(command->operands)) > 0)
        clp_invalid_arg_exit("command '%s' cannot have both operands and subcommands\n", command->name.data);

    sub_command->parent_command = command;
    if (d_dyn_array_push_back_ptr(&command->sub_commands, sub_command) != D_OK)
        clp_eprint_exit("out of memory\n");
}

void clp_add_command_option(Command *command, Option *command_option)
{
    if (command == NULL || command_option == NULL)
        clp_invalid_arg_exit("null argument to clp_add_command_option\n");
    if (clp_get_option_by_long(command, command_option->long_name))
        clp_eprint_exit("command %s: option '--%s' already registered\n", command->name.data, command_option->long_name.data);
    if (clp_get_option_by_short(command, command_option->short_name))
        clp_eprint_exit("command %s: option '-%c' already registered\n", command->name.data, command_option->short_name);
    if (d_dyn_array_push_back_ptr(&command->options, command_option) != D_OK)
        clp_eprint_exit("out of memory\n");
}

void clp_add_command_operand(Command *command, Operand *command_operand)
{
    if (command == NULL || command_operand == NULL)
        clp_invalid_arg_exit("null argument to clp_add_command_operand\n");
    if (d_dyn_array_get_size_safe(&(command->sub_commands)) > 0)
        clp_invalid_arg_exit("command '%s' cannot have both operands and subcommands\n", command->name.data);

    DDynArray *ops = &command->operands;
    usize size = d_dyn_array_get_size_safe(ops);
    Operand *last_operand = size == 0 ? NULL : d_dyn_array_get_elem_deref_addr_at_safe(ops, size - 1);

    if (get_command_operand_by_name(command, command_operand->name))
        clp_eprint("warning: operand name '%s' already used, consider a more descriptive name\n", command_operand->name.data);
    else if (last_operand != NULL && (last_operand->required == false && command_operand->required == true))
        clp_eprint_exit("command %s: required operand '%s' cannot follow optional operand '%s'\n", command->name.data, command_operand->name.data, last_operand->name.data);
    if (d_dyn_array_push_back_ptr(&command->operands, command_operand) != D_OK)
        clp_eprint_exit("out of memory\n");
}

static usize opt_name_width(Option *opt)
{
    usize w = 0;
    bool has_short = opt->short_name != (char)FLAG_SHORT_OPT_NOT_SET;
    bool has_long = opt->long_name.size > 0;

    if (has_short)
        w += 2; /* -X */
    if (has_short && has_long)
        w += 2; /* ", " */
    else if (!has_short && has_long)
        w += 4; /* leading spaces to align with "-X, " */
    if (has_long)
        w += 2 + opt->long_name.size; /* --name */
    if (opt->has_args)
        w += 2 + strlen(type_to_str(opt->type)); /* <type> */
    return w;
}

static usize sub_cmd_name_width(Command *cmd)
{
    return cmd->name.size;
}

static usize operand_name_width(Operand *op)
{
    return op->name.size + 2; /* <name> */
}

static usize max_col_width(Command *root)
{
    usize max = 0;

    DDynArray *opts = &root->options;
    usize size = d_dyn_array_get_size_safe(opts);
    for (usize i = 0; i < size; i++)
    {
        Option *opt = d_dyn_array_get_elem_deref_addr_at_safe(opts, i);
        usize w = opt_name_width(opt);
        if (w > max)
            max = w;
    }

    DDynArray *cmds = &root->sub_commands;
    size = d_dyn_array_get_size_safe(cmds);
    for (usize i = 0; i < size; i++)
    {
        Command *sub = d_dyn_array_get_elem_deref_addr_at_safe(cmds, i);
        usize w = sub_cmd_name_width(sub);
        if (w > max)
            max = w;
    }

    DDynArray *ops = &root->operands;
    size = d_dyn_array_get_size_safe(ops);
    for (usize i = 0; i < size; i++)
    {
        Operand *op = d_dyn_array_get_elem_deref_addr_at_safe(ops, i);
        usize w = operand_name_width(op);
        if (w > max)
            max = w;
    }

    return max;
}

static void print_options(DDynArray *opts, usize col_width)
{
    usize n = d_dyn_array_get_size_safe(opts);
    for (usize i = 0; i < n; i++)
    {
        Option *opt = d_dyn_array_get_elem_deref_addr_at_safe(opts, i);
        bool has_short = opt->short_name != (char)FLAG_SHORT_OPT_NOT_SET;
        bool has_long = opt->long_name.size > 0;
        usize written = 0;

        printf("  ");
        if (has_short)
        {
            printf("-%c", opt->short_name);
            written += 2;
            if (has_long)
            {
                printf(", ");
                written += 2;
            }
        }
        else if (has_long)
        {
            printf("    ");
            written += 4;
        }
        if (has_long)
        {
            printf("--%.*s", (int)opt->long_name.size, opt->long_name.data);
            written += 2 + opt->long_name.size;
        }
        if (opt->has_args)
        {
            const char *ts = type_to_str(opt->type);
            printf(" <%s>", ts);
            written += 2 + strlen(ts);
        }
        printf("%*s%s\n", (int)(col_width - written + HELP_COL_GAP), "", opt->description);
    }
}

static void print_sub_commands(DDynArray *cmds, usize col_width)
{
    usize n = d_dyn_array_get_size_safe(cmds);
    for (usize i = 0; i < n; i++)
    {
        Command *sub = d_dyn_array_get_elem_deref_addr_at_safe(cmds, i);
        printf("  %.*s%*s%s\n",
               (int)sub->name.size, sub->name.data,
               (int)(col_width - sub->name.size + HELP_COL_GAP), "",
               sub->description);
    }
}

static void print_operands(DDynArray *ops, usize col_width)
{
    usize n = d_dyn_array_get_size_safe(ops);
    for (usize i = 0; i < n; i++)
    {
        Operand *op = d_dyn_array_get_elem_deref_addr_at_safe(ops, i);
        usize name_w = op->name.size + 2;
        printf("  <%.*s>%*s%s\n",
               (int)op->name.size, op->name.data,
               (int)(col_width - name_w + HELP_COL_GAP), "",
               op->description);
    }
}

static void print_command_name_from_root_to_current(FILE *stream, char separator, Command *current)
{
    if (current->parent_command)
        print_command_name_from_root_to_current(stream, separator, current->parent_command);
    fprintf(stream, "%s%c", current->name.data, separator);
}

static void print_usage_line(FILE *stream, Command *command)
{
    fprintf(stream, "Usage: ");
    print_command_name_from_root_to_current(stream, ' ', command);
    fprintf(stream, "[OPTIONS] ");

    DDynArray *options = &command->options;
    for (size_t i = 0; i < d_dyn_array_get_size_safe(&(command->options)); i++)
    {
        Option *option = d_dyn_array_get_elem_deref_addr_at_safe(options, i);
        if (option->long_name.size != 0)
            fprintf(stream, "--%s", option->long_name.data);
        else
            fprintf(stream, "-%c", option->short_name);
        fprintf(stream, " <%s> ", type_to_str(option->type));
    }

    bool has_cmd = d_dyn_array_get_size_safe(&(command->sub_commands)) > 0;
    bool has_operands = d_dyn_array_get_size_safe(&(command->operands)) > 0;
    if (has_cmd)
        fprintf(stream, "<COMMAND>");
    else if (has_operands)
    {
        DDynArray *operands = &command->operands;
        for (size_t i = 0; i < d_dyn_array_get_size_safe(&(command->operands)); i++)
        {
            Operand *operand = d_dyn_array_get_elem_deref_addr_at_safe(operands, i);
            fprintf(stream, "<%s>", operand->name.data);
            if (operand->action == OPERAND_ACT_LIST)
                fprintf(stream, "...");
            fprintf(stream, " ");
        }
    }

    fprintf(stream, "\n\n");
}

static bool print_required_opts_if_missing(Command *root)
{
    DDynArray *opts = &root->options;
    usize size = d_dyn_array_get_size_safe(opts);
    bool required_opts_not_set = false;
    for (size_t i = 0; i < size; i++)
    {
        Option *opt = d_dyn_array_get_elem_deref_addr_at_safe(opts, i);
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
    fprintf(stderr, "\n");
    return required_opts_not_set;
}

static bool print_required_operands_if_missing(Command *root)
{
    DDynArray *operands = &root->operands;
    usize size = d_dyn_array_get_size_safe(operands);
    bool required_operands_not_set = false;
    for (size_t i = 0; i < size; i++)
    {
        Operand *operand = d_dyn_array_get_elem_deref_addr_at_safe(operands, i);
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
    fprintf(stderr, "\n");
    return required_operands_not_set;
}

static void print_command_help(Command *root)
{
    usize col = max_col_width(root);

    printf("Description: %s\n\n", root->description);

    bool has_opts = d_dyn_array_get_size_safe(&root->options) > 0;
    bool has_cmds = d_dyn_array_get_size_safe(&root->sub_commands) > 0;
    bool has_ops = d_dyn_array_get_size_safe(&root->operands) > 0;

    if (has_opts)
    {
        printf("Options:\n");
        print_options(&root->options, col);
        printf("\n");
    }
    if (has_cmds)
    {
        printf("Commands:\n");
        print_sub_commands(&root->sub_commands, col);
        printf("\n");
    }
    if (has_ops)
    {
        printf("Arguments:\n");
        print_operands(&root->operands, col);
        printf("\n");
    }
}

static void exit_print_help(Command *root)
{
    print_usage_line(stdout, root);
    print_command_help(root);
    exit(EXIT_SUCCESS);
}

static inline void exit_help_or_if_invalid_opt(Command *root, Option *opt, char *prefix, const char *name, usize len)
{
    if (len == sizeof(HELP_OPT) - 1 && PSEUDO_FAST_STRCMP(name, HELP_OPT))
        exit_print_help(root);

    if (opt == NULL)
    {
        if (prefix[1] == 0 && name[0] == HELP_OPT[0]) // if prefix[1] == 0 then we know it is a short option
            exit_print_help(root);                    // printing command help only if user has not define a short option with 'h'
        clp_eprint_exit("command %s: unknown option '%s%.*s'\n", root->name.data, prefix, (int)len, name);
    }
}

static void exit_if_command_required_nb_opts_not_met(Command *root)
{
    if (print_required_opts_if_missing(root) == true)
        exit(EXIT_FAILURE);
}

static void exit_if_command_required_nb_operands_size_not_met(Command *root)
{
    if (print_required_operands_if_missing(root) == true)
        exit(EXIT_FAILURE);
}

static void exit_print_usage_if_misused_command(Command *command)
{
    if (print_required_opts_if_missing(command) == false && print_required_operands_if_missing(command) == false)
        return;
    print_usage_line(stderr, command);
    fprintf(stderr, "For more information, try '--help'.\n");
    exit(EXIT_FAILURE);
}

static void set_opt_value(Command *root, Option *opt, const char *operand, char *prefix, const char *opt_name, usize len_name)
{
    ConversionFn conversion_fn = type_to_conversion_fn(opt->type);

    switch (opt->action)
    {
    case ARG_ACT_SET:
        if (opt->type != TYPE_BOOL)
        {
            if (operand == NULL)
                clp_eprint_exit("command %s: '%s%.*s' option require an argument", root->name.data, prefix, (int)len_name, opt_name);
            char *err_msg = conversion_fn(operand, &opt->value);
            if (err_msg)
                clp_eprint_exit("command %s: invalid value '%s' for '%s%.*s': %s", root->name.data, operand, prefix, (int)len_name, opt_name, err_msg);
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
                clp_eprint_exit("error: %s", d_error_print_result_as_str(result));
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
        clp_eprint_exit("command %s: too many operands provided\n", root->name.data);
    else if (raw_operand == NULL)
        return argv;

    Operand *operand = d_dyn_array_get_elem_deref_addr_at_safe(operands, cursor);
    ConversionFn conversion_fn = type_to_conversion_fn(operand->type);

    switch (operand->action)
    {
    case OPERAND_ACT_SET:
        char *err_msg = conversion_fn(raw_operand, &operand->value);
        if (err_msg)
            clp_eprint_exit("command %s: invalid value '%s' for '<%s>': %s", root->name.data, raw_operand, operand->name.data, err_msg);
        argv++;
        break;
    case OPERAND_ACT_LIST:
        while (1)
        {
            DStringView list = d_string_view_from_c_string(raw_operand);
            DResult result;
            if ((result = d_dyn_array_push_back(&operand->value.value_list, &list)) != D_OK)
                clp_eprint_exit("%s", d_error_print_result_as_str(result));
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
    exit_help_or_if_invalid_opt(root, opt, DOUBLE_HYPHEN, long_opt.data, long_opt.size);
    if (has_inline_value && opt->has_args == false)
        clp_eprint_exit("command %s: option '--%s' doesn't allow an argument\n", root->name.data, opt->long_name.data);
    const char *operand = has_inline_value ? &long_opt.data[eq_pos + 1] : *argv;
    set_opt_value(root, opt, operand, DOUBLE_HYPHEN, long_opt.data, long_opt.size);
    return argv + (*argv != NULL && has_inline_value == false && opt->has_args == true);
}

static char **parse_short_opts(Command *root, char *short_opt, char **argv)
{
    usize i = 0;
    do
    {
        Option *opt = clp_get_option_by_short(root, short_opt[i]);
        exit_help_or_if_invalid_opt(root, opt, HYPHEN, &short_opt[i], 1);
        if (opt->has_args == true)
        {
            bool consume_next_argv = short_opt[i + 1] == 0;
            char *operand = consume_next_argv == true ? *argv : &short_opt[i + 1];
            set_opt_value(root, opt, operand, HYPHEN, &short_opt[i], 1);
            return argv + (*argv != NULL && consume_next_argv == true);
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
        exit_print_usage_if_misused_command(root);
        return;
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
                Command *sub_cmd = d_dyn_array_get_elem_deref_addr_at_safe(sub_commands, i);
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

void clp_parse_args(Command *root, char **argv, Command **command)
{
    if (root == NULL || argv == NULL || *argv == NULL)
        clp_invalid_arg_exit("null argument to clp_parse_args\n");
    parse(root, argv, command);
}

void free_command(void *command)
{
    if (command == NULL)
        return;
    Command *cmd = command;
    d_dyn_array_destroy(&cmd->sub_commands);
    DDynArray *opts = &cmd->options;
    for (usize i = 0; i < opts->array.size; i++)
    {
        Option *opt = d_dyn_array_get_elem_deref_addr_at_safe(opts, i);
        if (opt->type == TYPE_KV)
            d_unordered_map_destroy(&opt->value.value_kv);
        else if (opt->action == ARG_ACT_LIST)
            d_dyn_array_destroy(&opt->value.value_list);
    }

    d_dyn_array_destroy(&cmd->options);

    DDynArray *operands = &cmd->operands;
    for (usize i = 0; i < operands->array.size; i++)
    {
        Operand *operand = d_dyn_array_get_elem_deref_addr_at_safe(operands, i);
        if (operand->type == TYPE_KV)
            d_unordered_map_destroy(&operand->value.value_kv);
        else if (operand->action == OPERAND_ACT_LIST)
            d_dyn_array_destroy(&operand->value.value_list);
    }

    d_dyn_array_destroy(&cmd->operands);
    d_dyn_array_destroy(&cmd->extra);
}

static void _free_command(void **command)
{
    free_command(*command);
}

void clp_cleanup(Command *root)
{
    if (root == NULL)
        return;
    DDynArray *sub_commands = &root->sub_commands;
    usize size = d_dyn_array_get_size_safe(sub_commands);
    for (usize i = 0; i < size; i++)
        clp_cleanup(d_dyn_array_get_elem_deref_addr_at_safe(sub_commands, i));
    free_command(root);
}
