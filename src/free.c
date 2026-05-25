#include "free.h"

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
        if (opt->action == OPT_ACT_KV)
            d_unordered_map_destroy(&opt->value.value_kv);
        else if (opt->action == OPT_ACT_LIST)
            d_dyn_array_destroy(&opt->value.value_list);
    }

    d_dyn_array_destroy(&cmd->options);

    DDynArray *operands = &cmd->operands;
    for (usize i = 0; i < operands->array.size; i++)
    {
        Operand *operand = d_dyn_array_get_elem_deref_addr_at_safe(operands, i);
        if (operand->action == OPND_ACT_KV)
            d_unordered_map_destroy(&operand->value.value_kv);
        else if (operand->action == OPND_ACT_LIST)
            d_dyn_array_destroy(&operand->value.value_list);
    }

    d_dyn_array_destroy(&cmd->operands);
    d_dyn_array_destroy(&cmd->extra);
}
