#ifndef PARSE_H
#define PARSE_H
#include "clp.h"

void parse(Command *root, char **argv, Command **command);
void exit_if_not_valid_long_opt_name(DStringView long_opt);

#endif