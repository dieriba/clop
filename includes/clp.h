#ifndef CLP_H
#define CLP_H

#include "d_types.h"
#include "d_dyn_array.h"
#include "d_error.h"

#define clp_init_option(opt, long_name, short_name, description, value_type, required, global) \
    clp_init_option_raw(opt, long_name, short_name, description, false, (Value){0}, value_type, required, global)

#define clp_init_option_default_long(opt, ln, sn, desc, def, req, glob) \
    clp_init_option_raw(opt, ln, sn, desc, true, (Value){.value_long = (def)}, OPT_TYPE_LONG, req, glob)

#define clp_init_option_default_bool(opt, ln, sn, desc, def, req, glob) \
    clp_init_option_raw(opt, ln, sn, desc, true, (Value){.value_bool = (def)}, OPT_TYPE_BOOL, req, glob)

#define clp_init_option_default_usize(opt, ln, sn, desc, def, req, glob) \
    clp_init_option_raw(opt, ln, sn, desc, true, (Value){.value_usize = (def)}, OPT_TYPE_USIZE, req, glob)

#define clp_init_option_default_str(opt, ln, sn, desc, def, req, glob) \
    clp_init_option_raw(opt, ln, sn, desc, true, (Value){.value_str = (def)}, OPT_TYPE_STR, req, glob)

#define clp_init_option_default_char(opt, ln, sn, desc, def, req, glob) \
    clp_init_option_raw(opt, ln, sn, desc, true, (Value){.value_char = (def)}, OPT_TYPE_CHAR, req, glob)

#define clp_init_option_default_double(opt, ln, sn, desc, def, req, glob) \
    clp_init_option_raw(opt, ln, sn, desc, true, (Value){.value_double = (def)}, OPT_TYPE_DOUBLE, req, glob)

typedef enum ValueType
{
    OPT_TYPE_LONG,
    OPT_TYPE_BOOL,
    OPT_TYPE_USIZE,
    OPT_TYPE_STR,
    OPT_TYPE_CHAR,
    OPT_TYPE_DOUBLE
} ValueType;

typedef union Value
{
    long value_long;
    usize value_usize;
    bool value_bool;
    char *value_str;
    char value_char;
    double value_double;
} Value;

typedef struct CommandOption
{
    char *long_name;
    char short_name;
    char *description;
    bool required;
    bool has_default_value;
    bool global;
    Value value;
    ValueType value_type;

} CommandOption;

typedef struct Command
{
    char *name;
    char *description;
    int command_code;
    DDynArray options;
    DDynArray sub_commands;
} Command;

typedef struct Clp
{
    Command root;
} Clp;

DResult clp_init(Clp *clp, Command *root_command);
DResult clp_init_command(Command *command, char *name, char *description, int command_code);
DResult clp_add_command_sub_command(Command *command, Command *sub_command);
DResult clp_init_option_raw(CommandOption *opt, char *long_name, char short_name, char *description, bool has_default_value, Value value, ValueType type, bool required, bool global);
DResult clp_add_command_option(Command *command, CommandOption *command_option);

DResult clp_parse(Clp *clp, int argc, char **argv, DError *error);

DResult clp_cleanup(Clp *clp);
#endif