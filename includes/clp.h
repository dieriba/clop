#ifndef CLP_H
#define CLP_H

#include "d_types.h"
#include "d_dyn_array.h"
#include "d_string_view.h"
#include "d_error.h"
#include "d_unordered_map.h"

#define clp_init_option(opt, long_name, short_name, description, type, required, global) \
    clp_init_option_raw(opt, long_name, short_name, description, false, (Value){0}, type, required, global)

#define clp_init_option_default_long(opt, ln, sn, desc, def, req, glob) \
    clp_init_option_raw(opt, ln, sn, desc, true, (Value){.value_long = (def)}, TYPE_LONG, req, glob)

#define clp_init_option_default_bool(opt, ln, sn, desc, def, req, glob) \
    clp_init_option_raw(opt, ln, sn, desc, true, (Value){.value_bool = (def)}, TYPE_BOOL, req, glob)

#define clp_init_option_default_usize(opt, ln, sn, desc, def, req, glob) \
    clp_init_option_raw(opt, ln, sn, desc, true, (Value){.value_usize = (def)}, TYPE_USIZE, req, glob)

#define clp_init_option_default_str(opt, ln, sn, desc, def, req, glob) \
    clp_init_option_raw(opt, ln, sn, desc, true, (Value){.value_str = (def)}, TYPE_STR, req, glob)

#define clp_init_option_default_char(opt, ln, sn, desc, def, req, glob) \
    clp_init_option_raw(opt, ln, sn, desc, true, (Value){.value_char = (def)}, TYPE_CHAR, req, glob)

#define clp_init_option_default_double(opt, ln, sn, desc, def, req, glob) \
    clp_init_option_raw(opt, ln, sn, desc, true, (Value){.value_double = (def)}, TYPE_DOUBLE, req, glob)

typedef enum Type
{
    TYPE_LONG,
    TYPE_BOOL,
    TYPE_USIZE,
    TYPE_STR,
    TYPE_CHAR,
    TYPE_DOUBLE,
    TYPE_KV
} Type;

typedef enum OptAction
{
    ARG_ACT_COUNT,
    ARG_ACT_LIST,
    ARG_ACT_SET
} OptAction;

typedef enum OperanAction
{
    OPERAND_ACT_LIST,
    OPERAND_ACT_SET
} OperanAction;

typedef union Value
{
    DUnorderedMap value_kv;
    DDynArray value_list;
    usize value_usize;
    double value_double;
    long value_long;
    const char *value_str;
    bool value_bool;
    char value_char;
} Value;

typedef struct Operand
{
    Value value;
    DStringView name;
    char *description;
    OperanAction action;
    Type type;
    bool required;
    bool has_default_value;
    bool value_set;
} Operand;

typedef struct Option
{
    Value value;
    DStringView long_name;
    char *description;
    OptAction action;
    Type type;
    char short_name;
    bool required;
    bool has_default_value;
    bool value_set;
    bool global;
    bool has_args;
} Option;

typedef struct Command Command;

struct Command
{
    DDynArray options;
    DDynArray operands;
    DDynArray sub_commands;
    DDynArray extra;
    Command *parent_command;
    DStringView name;
    char *description;
    int code;
};

void clp_init_command(Command *command, int code, char *name, char *description);
void clp_add_command_sub_command(Command *command, Command *sub_command);
void clp_add_command_option(Command *command, Option *command_option);
void clp_add_command_operand(Command *command, Operand *command_operand);
void clp_init_option_raw(Option *opt, char *long_name, char *short_name, char *description, bool has_default_value, OptAction action, Value value, Type type, bool required, bool global);
void clp_init_operand_raw(Operand *operands, char *name, char *description, bool has_default_value, OperanAction action, Value value, Type type, bool required);
void clp_parse_args(Command *root, char **argv, Command **command);
void print_command_help(Command *root);
Option *clp_get_option_by_short(Command *command, char shrt);
Option *clp_get_option_by_long(Command *command, DStringView lng);
Operand *clp_get_operand(Command *command, DStringView operand_name);
void clp_cleanup(Command *root);
#endif