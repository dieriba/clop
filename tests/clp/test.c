#include <string.h>
#include <stdbool.h>
#include "raw_map.h"
#include "d_test.h"
#include "clp.h"

/* ── helpers ────────────────────────────────────────────────────────────── */

static void init_bool_opt(Option *opt, char *lng, char *sht, bool required)
{
    D_TEST_EXPR(clp_init_option_raw(opt, lng, sht, NULL, false,
                                    ARG_ACT_SET, (Value){0},
                                    TYPE_BOOL, required, false) == D_OK);
}

static void init_str_opt(Option *opt, char *lng, char *sht, bool required)
{
    D_TEST_EXPR(clp_init_option_raw(opt, lng, sht, NULL, false,
                                    ARG_ACT_SET, (Value){0},
                                    TYPE_STR, required, false) == D_OK);
}

static void init_count_opt(Option *opt, char *lng, char *sht)
{
    D_TEST_EXPR(clp_init_option_raw(opt, lng, sht, NULL, false,
                                    ARG_ACT_COUNT, (Value){0},
                                    TYPE_USIZE, false, false) == D_OK);
}

static void init_list_opt(Option *opt, char *lng, char *sht)
{
    Value v = {0};
    D_TEST_EXPR(d_dyn_array_default_init(&v.value_list, DStringView, NULL, NULL, RAW_BUF_OPT_NONE) == D_OK);
    D_TEST_EXPR(clp_init_option_raw(opt, lng, sht, NULL, false,
                                    ARG_ACT_LIST, v,
                                    TYPE_STR, false, false) == D_OK);
}

static void init_str_operand(Operand *op, char *name, bool required)
{
    D_TEST_EXPR(clp_init_operand_raw(op, name, NULL, false,
                                     OPERAND_ACT_SET, (Value){0},
                                     TYPE_STR, required) == D_OK);
}

static void init_list_operand(Operand *op, char *name, bool required)
{
    Value v = {0};
    D_TEST_EXPR(d_dyn_array_default_init(&v.value_list, DStringView, NULL, NULL, RAW_BUF_OPT_NONE) == D_OK);
    D_TEST_EXPR(clp_init_operand_raw(op, name, NULL, false,
                                     OPERAND_ACT_LIST, v,
                                     TYPE_STR, required) == D_OK);
}

/* ── null-guard tests ───────────────────────────────────────────────────── */

static void test_init_command_rejects_null_command(void)
{
    D_TEST_EXPR(clp_init_command(NULL, 0, "prog", NULL) == D_ERR_INVALID_ARG);
}

static void test_init_command_rejects_null_name(void)
{
    Command cmd;
    D_TEST_EXPR(clp_init_command(&cmd, 0, NULL, NULL) == D_ERR_INVALID_ARG);
}

static void test_add_option_rejects_null_command(void)
{
    Option opt;
    init_bool_opt(&opt, "verbose", "v", false);
    D_TEST_EXPR(clp_add_command_option(NULL, &opt) == D_ERR_INVALID_ARG);
}

static void test_add_option_rejects_null_option(void)
{
    Command cmd;
    D_TEST_EXPR(clp_init_command(&cmd, 0, "prog", NULL) == D_OK);
    D_TEST_EXPR(clp_add_command_option(&cmd, NULL) == D_ERR_INVALID_ARG);
    clp_cleanup(&cmd);
}

static void test_add_operand_rejects_null_command(void)
{
    Operand op;
    init_str_operand(&op, "file", false);
    D_TEST_EXPR(clp_add_command_operand(NULL, &op) == D_ERR_INVALID_ARG);
}

static void test_add_operand_rejects_null_operand(void)
{
    Command cmd;
    D_TEST_EXPR(clp_init_command(&cmd, 0, "prog", NULL) == D_OK);
    D_TEST_EXPR(clp_add_command_operand(&cmd, NULL) == D_ERR_INVALID_ARG);
    clp_cleanup(&cmd);
}

static void test_parse_args_rejects_null_root(void)
{
    char *argv[] = {"prog", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(NULL, argv, &cmd) == D_ERR_INVALID_ARG);
}

static void test_parse_args_rejects_null_argv(void)
{
    Command root;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, NULL, &cmd) == D_ERR_INVALID_ARG);
    clp_cleanup(&root);
}

static void test_get_option_by_short_returns_null_for_null_command(void)
{
    D_TEST_NULL(clp_get_option_by_short(NULL, 'v'));
}

static void test_get_option_by_long_returns_null_for_null_command(void)
{
    D_TEST_NULL(clp_get_option_by_long(NULL, D_STRING_VIEW_FROM_LITERAL("verbose")));
}

static void test_get_option_by_long_returns_null_for_empty_view(void)
{
    Command cmd;
    D_TEST_EXPR(clp_init_command(&cmd, 0, "prog", NULL) == D_OK);
    D_TEST_NULL(clp_get_option_by_long(&cmd, D_STRING_VIEW_FROM_LITERAL("")));
    clp_cleanup(&cmd);
}

static void test_get_operand_returns_null_for_null_command(void)
{
    D_TEST_NULL(clp_get_operand(NULL, D_STRING_VIEW_FROM_LITERAL("file")));
}

/* ── getter tests ───────────────────────────────────────────────────────── */

static void test_get_option_by_short_finds_registered_option(void)
{
    Command cmd;
    Option opt;
    D_TEST_EXPR(clp_init_command(&cmd, 0, "prog", NULL) == D_OK);
    init_bool_opt(&opt, "verbose", "v", false);
    D_TEST_EXPR(clp_add_command_option(&cmd, &opt) == D_OK);
    D_TEST_EXPR(clp_get_option_by_short(&cmd, 'v') == &opt);
    clp_cleanup(&cmd);
}

static void test_get_option_by_short_returns_null_for_unknown(void)
{
    Command cmd;
    Option opt;
    D_TEST_EXPR(clp_init_command(&cmd, 0, "prog", NULL) == D_OK);
    init_bool_opt(&opt, "verbose", "v", false);
    D_TEST_EXPR(clp_add_command_option(&cmd, &opt) == D_OK);
    D_TEST_NULL(clp_get_option_by_short(&cmd, 'x'));
    clp_cleanup(&cmd);
}

static void test_get_option_by_long_finds_registered_option(void)
{
    Command cmd;
    Option opt;
    D_TEST_EXPR(clp_init_command(&cmd, 0, "prog", NULL) == D_OK);
    init_bool_opt(&opt, "verbose", "v", false);
    D_TEST_EXPR(clp_add_command_option(&cmd, &opt) == D_OK);
    D_TEST_EXPR(clp_get_option_by_long(&cmd, D_STRING_VIEW_FROM_LITERAL("verbose")) == &opt);
    clp_cleanup(&cmd);
}

static void test_get_option_by_long_returns_null_for_unknown(void)
{
    Command cmd;
    Option opt;
    D_TEST_EXPR(clp_init_command(&cmd, 0, "prog", NULL) == D_OK);
    init_bool_opt(&opt, "verbose", "v", false);
    D_TEST_EXPR(clp_add_command_option(&cmd, &opt) == D_OK);
    D_TEST_NULL(clp_get_option_by_long(&cmd, D_STRING_VIEW_FROM_LITERAL("quiet")));
    clp_cleanup(&cmd);
}

static void test_get_operand_finds_registered_operand(void)
{
    Command cmd;
    Operand op;
    D_TEST_EXPR(clp_init_command(&cmd, 0, "prog", NULL) == D_OK);
    init_str_operand(&op, "file", false);
    D_TEST_EXPR(clp_add_command_operand(&cmd, &op) == D_OK);
    D_TEST_EXPR(clp_get_operand(&cmd, D_STRING_VIEW_FROM_LITERAL("file")) == &op);
    clp_cleanup(&cmd);
}

static void test_get_operand_returns_null_for_unknown(void)
{
    Command cmd;
    Operand op;
    D_TEST_EXPR(clp_init_command(&cmd, 0, "prog", NULL) == D_OK);
    init_str_operand(&op, "file", false);
    D_TEST_EXPR(clp_add_command_operand(&cmd, &op) == D_OK);
    D_TEST_NULL(clp_get_operand(&cmd, D_STRING_VIEW_FROM_LITERAL("output")));
    clp_cleanup(&cmd);
}

/* ── bool option parsing ────────────────────────────────────────────────── */

static void test_long_bool_flag_sets_value(void)
{
    Command root;
    Option verbose;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_bool_opt(&verbose, "verbose", "v", false);
    D_TEST_EXPR(clp_add_command_option(&root, &verbose) == D_OK);

    char *argv[] = {"prog", "--verbose", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(verbose.value_set == true);
    D_TEST_EXPR(verbose.value.value_bool == true);
    clp_cleanup(&root);
}

static void test_short_bool_flag_sets_value(void)
{
    Command root;
    Option verbose;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_bool_opt(&verbose, "verbose", "v", false);
    D_TEST_EXPR(clp_add_command_option(&root, &verbose) == D_OK);

    char *argv[] = {"prog", "-v", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(verbose.value_set == true);
    D_TEST_EXPR(verbose.value.value_bool == true);
    clp_cleanup(&root);
}

static void test_unprovided_optional_bool_stays_unset(void)
{
    Command root;
    Option verbose;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_bool_opt(&verbose, "verbose", "v", false);
    D_TEST_EXPR(clp_add_command_option(&root, &verbose) == D_OK);

    char *argv[] = {"prog", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(verbose.value_set == false);
    D_TEST_EXPR(verbose.value.value_bool == false);
    clp_cleanup(&root);
}

/* POSIX: multiple bool flags combined in one token: -abc */
static void test_combined_short_bool_flags(void)
{
    Command root;
    Option fa, fb, fc;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_bool_opt(&fa, "aaa", "a", false);
    init_bool_opt(&fb, "bbb", "b", false);
    init_bool_opt(&fc, "ccc", "c", false);
    D_TEST_EXPR(clp_add_command_option(&root, &fa) == D_OK);
    D_TEST_EXPR(clp_add_command_option(&root, &fb) == D_OK);
    D_TEST_EXPR(clp_add_command_option(&root, &fc) == D_OK);

    char *argv[] = {"prog", "-abc", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(fa.value_set == true);
    D_TEST_EXPR(fb.value_set == true);
    D_TEST_EXPR(fc.value_set == true);
    D_TEST_EXPR(fa.value.value_bool == true);
    D_TEST_EXPR(fb.value.value_bool == true);
    D_TEST_EXPR(fc.value.value_bool == true);
    clp_cleanup(&root);
}

/* Parsing multiple separate bool flags */
static void test_multiple_separate_short_bool_flags(void)
{
    Command root;
    Option fa, fb;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_bool_opt(&fa, "aaa", "a", false);
    init_bool_opt(&fb, "bbb", "b", false);
    D_TEST_EXPR(clp_add_command_option(&root, &fa) == D_OK);
    D_TEST_EXPR(clp_add_command_option(&root, &fb) == D_OK);

    char *argv[] = {"prog", "-a", "-b", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(fa.value_set == true);
    D_TEST_EXPR(fb.value_set == true);
    clp_cleanup(&root);
}

/* Only first flag in combined token is set when second is unknown */
static void test_combined_flags_only_set_found_options(void)
{
    Command root;
    Option fa, fb;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_bool_opt(&fa, "aaa", "a", false);
    init_bool_opt(&fb, "bbb", "b", false);
    D_TEST_EXPR(clp_add_command_option(&root, &fa) == D_OK);
    D_TEST_EXPR(clp_add_command_option(&root, &fb) == D_OK);

    char *argv[] = {"prog", "-a", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(fa.value_set == true);
    D_TEST_EXPR(fb.value_set == false);
    clp_cleanup(&root);
}

/* ── non-bool options: value_set and value correctness ─────────────────── */

/* POSIX: -oVALUE (value is remainder of token, no space) */
static void test_short_opt_inline_value(void)
{
    Command root;
    Option output;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_str_opt(&output, "output", "o", false);
    D_TEST_EXPR(clp_add_command_option(&root, &output) == D_OK);

    char *argv[] = {"prog", "-ofile.txt", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(output.value_set == true);
    D_TEST_STR_EQ(output.value.value_str, "file.txt");
    clp_cleanup(&root);
}

/* POSIX: -o VALUE (value is next argv token) */
static void test_short_opt_next_argv_value(void)
{
    Command root;
    Option output;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_str_opt(&output, "output", "o", false);
    D_TEST_EXPR(clp_add_command_option(&root, &output) == D_OK);

    char *argv[] = {"prog", "-o", "file.txt", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(output.value_set == true);
    D_TEST_STR_EQ(output.value.value_str, "file.txt");
    clp_cleanup(&root);
}

/* GNU: --output=VALUE */
static void test_long_opt_inline_eq_value(void)
{
    Command root;
    Option output;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_str_opt(&output, "output", "o", false);
    D_TEST_EXPR(clp_add_command_option(&root, &output) == D_OK);

    char *argv[] = {"prog", "--output=file.txt", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(output.value_set == true);
    D_TEST_STR_EQ(output.value.value_str, "file.txt");
    clp_cleanup(&root);
}

/* GNU: --output VALUE */
static void test_long_opt_next_argv_value(void)
{
    Command root;
    Option output;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_str_opt(&output, "output", "o", false);
    D_TEST_EXPR(clp_add_command_option(&root, &output) == D_OK);

    char *argv[] = {"prog", "--output", "file.txt", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(output.value_set == true);
    D_TEST_STR_EQ(output.value.value_str, "file.txt");
    clp_cleanup(&root);
}

/* POSIX: -abVALUE — combined bools followed by value-taking option inline */
static void test_combined_bools_then_inline_value_opt(void)
{
    Command root;
    Option fa, fb, output;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_bool_opt(&fa, "aaa", "a", false);
    init_bool_opt(&fb, "bbb", "b", false);
    init_str_opt(&output, "output", "o", false);
    D_TEST_EXPR(clp_add_command_option(&root, &fa) == D_OK);
    D_TEST_EXPR(clp_add_command_option(&root, &fb) == D_OK);
    D_TEST_EXPR(clp_add_command_option(&root, &output) == D_OK);

    char *argv[] = {"prog", "-abofile.txt", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(fa.value_set == true);
    D_TEST_EXPR(fb.value_set == true);
    D_TEST_EXPR(output.value_set == true);
    D_TEST_STR_EQ(output.value.value_str, "file.txt");
    clp_cleanup(&root);
}

/* ── count options ──────────────────────────────────────────────────────── */

static void test_count_option_increments_once(void)
{
    Command root;
    Option verbose;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_count_opt(&verbose, "verbose", "v");
    D_TEST_EXPR(clp_add_command_option(&root, &verbose) == D_OK);

    char *argv[] = {"prog", "-v", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(verbose.value.value_usize == 1);
    clp_cleanup(&root);
}

static void test_count_option_increments_multiple_times(void)
{
    Command root;
    Option verbose;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_count_opt(&verbose, "verbose", "v");
    D_TEST_EXPR(clp_add_command_option(&root, &verbose) == D_OK);

    char *argv[] = {"prog", "-v", "-v", "-v", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(verbose.value.value_usize == 3);
    clp_cleanup(&root);
}

/* POSIX: -vvv combined should count 3 */
static void test_count_option_combined_short(void)
{
    Command root;
    Option verbose;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_count_opt(&verbose, "verbose", "v");
    D_TEST_EXPR(clp_add_command_option(&root, &verbose) == D_OK);

    char *argv[] = {"prog", "-vvv", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(verbose.value.value_usize == 3);
    clp_cleanup(&root);
}

static void test_count_option_mixed_short_and_long(void)
{
    Command root;
    Option verbose;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_count_opt(&verbose, "verbose", "v");
    D_TEST_EXPR(clp_add_command_option(&root, &verbose) == D_OK);

    char *argv[] = {"prog", "-v", "--verbose", "-vv", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(verbose.value.value_usize == 4);
    clp_cleanup(&root);
}

/* ── list options ───────────────────────────────────────────────────────── */

static void test_list_option_single_entry(void)
{
    Command root;
    Option files;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_list_opt(&files, "files", "f");
    D_TEST_EXPR(clp_add_command_option(&root, &files) == D_OK);

    char *argv[] = {"prog", "--files", "a.txt", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(files.value_set == true);
    D_TEST_EXPR(d_dyn_array_get_size_safe(&files.value.value_list) == 1);
    clp_cleanup(&root);
}

static void test_list_option_comma_separated(void)
{
    Command root;
    Option files;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_list_opt(&files, "files", "f");
    D_TEST_EXPR(clp_add_command_option(&root, &files) == D_OK);

    char *argv[] = {"prog", "--files", "a.txt,b.txt,c.txt", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(files.value_set == true);
    D_TEST_EXPR(d_dyn_array_get_size_safe(&files.value.value_list) == 3);

    DStringView tok;
    D_TEST_EXPR(d_dyn_array_get_elem_at(&files.value.value_list, 0, &tok) == D_OK);
    D_TEST_EXPR(d_string_view_compare_against_c_string(tok, "a.txt"));
    D_TEST_EXPR(d_dyn_array_get_elem_at(&files.value.value_list, 1, &tok) == D_OK);
    D_TEST_EXPR(d_string_view_compare_against_c_string(tok, "b.txt"));
    D_TEST_EXPR(d_dyn_array_get_elem_at(&files.value.value_list, 2, &tok) == D_OK);
    D_TEST_EXPR(d_string_view_compare_against_c_string(tok, "c.txt"));
    clp_cleanup(&root);
}

/* ── operand parsing ────────────────────────────────────────────────────── */

static void test_single_operand_is_set(void)
{
    Command root;
    Operand file;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_str_operand(&file, "file", false);
    D_TEST_EXPR(clp_add_command_operand(&root, &file) == D_OK);

    char *argv[] = {"prog", "input.txt", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(file.value_set == true);
    D_TEST_STR_EQ(file.value.value_str, "input.txt");
    clp_cleanup(&root);
}

static void test_options_and_operand_together(void)
{
    Command root;
    Option verbose;
    Operand file;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_bool_opt(&verbose, "verbose", "v", false);
    init_str_operand(&file, "file", false);
    D_TEST_EXPR(clp_add_command_option(&root, &verbose) == D_OK);
    D_TEST_EXPR(clp_add_command_operand(&root, &file) == D_OK);

    char *argv[] = {"prog", "-v", "input.txt", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(verbose.value_set == true);
    D_TEST_EXPR(file.value_set == true);
    clp_cleanup(&root);
}

/* GNU: options can appear after operands (non-POSIX scanning) */
static void test_option_after_operand(void)
{
    Command root;
    Option verbose;
    Operand file;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_bool_opt(&verbose, "verbose", "v", false);
    init_str_operand(&file, "file", false);
    D_TEST_EXPR(clp_add_command_option(&root, &verbose) == D_OK);
    D_TEST_EXPR(clp_add_command_operand(&root, &file) == D_OK);

    char *argv[] = {"prog", "input.txt", "-v", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(file.value_set == true);
    D_TEST_EXPR(verbose.value_set == true);
    clp_cleanup(&root);
}

static void test_multiple_operands(void)
{
    Command root;
    Operand src, dst;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_str_operand(&src, "src", false);
    init_str_operand(&dst, "dst", false);
    D_TEST_EXPR(clp_add_command_operand(&root, &src) == D_OK);
    D_TEST_EXPR(clp_add_command_operand(&root, &dst) == D_OK);

    char *argv[] = {"prog", "a.txt", "b.txt", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(src.value_set == true);
    D_TEST_EXPR(dst.value_set == true);
    D_TEST_STR_EQ(src.value.value_str, "a.txt");
    D_TEST_STR_EQ(dst.value.value_str, "b.txt");
    clp_cleanup(&root);
}

/* POSIX/GNU: -- terminates option parsing; everything after is an operand */
static void test_double_hyphen_terminates_option_parsing(void)
{
    Command root;
    Option verbose;
    Operand file;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_bool_opt(&verbose, "verbose", "v", false);
    init_str_operand(&file, "file", false);
    D_TEST_EXPR(clp_add_command_option(&root, &verbose) == D_OK);
    D_TEST_EXPR(clp_add_command_operand(&root, &file) == D_OK);

    char *argv[] = {"prog", "--", "--verbose", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(verbose.value_set == false);
    D_TEST_EXPR(file.value_set == true);
    D_TEST_STR_EQ(file.value.value_str, "--verbose");
    clp_cleanup(&root);
}

static void test_double_hyphen_with_options_before(void)
{
    Command root;
    Option verbose;
    Operand file;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_bool_opt(&verbose, "verbose", "v", false);
    init_str_operand(&file, "file", false);
    D_TEST_EXPR(clp_add_command_option(&root, &verbose) == D_OK);
    D_TEST_EXPR(clp_add_command_operand(&root, &file) == D_OK);

    char *argv[] = {"prog", "-v", "--", "file.txt", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(verbose.value_set == true);
    D_TEST_EXPR(file.value_set == true);
    D_TEST_STR_EQ(file.value.value_str, "file.txt");
    clp_cleanup(&root);
}

/* ── list operands ──────────────────────────────────────────────────────── */

static void test_list_operand_consumes_remaining_args(void)
{
    Command root;
    Operand files;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_list_operand(&files, "files", false);
    D_TEST_EXPR(clp_add_command_operand(&root, &files) == D_OK);

    char *argv[] = {"prog", "a.txt", "b.txt", "c.txt", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(files.value_set == true);
    D_TEST_EXPR(d_dyn_array_get_size_safe(&files.value.value_list) == 3);
    clp_cleanup(&root);
}

/* ── subcommand dispatch ────────────────────────────────────────────────── */

static void test_subcommand_is_dispatched(void)
{
    Command root, add;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&add, 1, "add", NULL) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &add) == D_OK);

    char *argv[] = {"prog", "add", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(cmd == &add);
    clp_cleanup(&root);
}

static void test_subcommand_with_own_option(void)
{
    Command root, commit;
    Option msg;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&commit, 1, "commit", NULL) == D_OK);
    init_bool_opt(&msg, "amend", "a", false);
    D_TEST_EXPR(clp_add_command_option(&commit, &msg) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &commit) == D_OK);

    char *argv[] = {"prog", "commit", "--amend", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(cmd == &commit);
    D_TEST_EXPR(msg.value_set == true);
    D_TEST_EXPR(msg.value.value_bool == true);
    clp_cleanup(&root);
}

static void test_no_subcommand_leaves_command_null(void)
{
    Command root, add;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&add, 1, "add", NULL) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &add) == D_OK);

    char *argv[] = {"prog", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_NULL(cmd);
    clp_cleanup(&root);
}

static void test_multiple_subcommands_dispatch_correct_one(void)
{
    Command root, add, rm;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&add, 1, "add", NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&rm, 2, "rm", NULL) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &add) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &rm) == D_OK);

    char *argv[] = {"prog", "rm", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(cmd == &rm);
    clp_cleanup(&root);
}

/* ── type conversions ───────────────────────────────────────────────────── */

static void test_usize_option_parses_decimal(void)
{
    Command root;
    Option jobs;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    D_TEST_EXPR(clp_init_option_raw(&jobs, "jobs", "j", NULL, false,
                                    ARG_ACT_SET, (Value){0},
                                    TYPE_USIZE, false, false) == D_OK);
    D_TEST_EXPR(clp_add_command_option(&root, &jobs) == D_OK);

    char *argv[] = {"prog", "--jobs", "4", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(jobs.value_set == true);
    D_TEST_EXPR(jobs.value.value_usize == 4);
    clp_cleanup(&root);
}

static void test_long_option_parses_negative(void)
{
    Command root;
    Option level;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    D_TEST_EXPR(clp_init_option_raw(&level, "level", "l", NULL, false,
                                    ARG_ACT_SET, (Value){0},
                                    TYPE_LONG, false, false) == D_OK);
    D_TEST_EXPR(clp_add_command_option(&root, &level) == D_OK);

    char *argv[] = {"prog", "--level", "-5", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(level.value_set == true);
    D_TEST_EXPR(level.value.value_long == -5);
    clp_cleanup(&root);
}

static void test_char_option_parses_single_char(void)
{
    Command root;
    Option sep;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    D_TEST_EXPR(clp_init_option_raw(&sep, "sep", "s", NULL, false,
                                    ARG_ACT_SET, (Value){0},
                                    TYPE_CHAR, false, false) == D_OK);
    D_TEST_EXPR(clp_add_command_option(&root, &sep) == D_OK);

    char *argv[] = {"prog", "--sep", ",", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(sep.value_set == true);
    D_TEST_EXPR(sep.value.value_char == ',');
    clp_cleanup(&root);
}

/* ── edge cases ─────────────────────────────────────────────────────────── */

/* --output= (empty value after =) */
static void test_long_opt_empty_inline_value(void)
{
    Command root;
    Option output;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_str_opt(&output, "output", "o", false);
    D_TEST_EXPR(clp_add_command_option(&root, &output) == D_OK);

    char *argv[] = {"prog", "--output=", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(output.value_set == true);
    D_TEST_STR_EQ(output.value.value_str, "");
    clp_cleanup(&root);
}

/* options mixed before and after operands (GNU scanning) */
static void test_option_interleaved_with_operands(void)
{
    Command root;
    Option verbose, dry;
    Operand src, dst;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_bool_opt(&verbose, "verbose", "v", false);
    init_bool_opt(&dry, "dry-run", "n", false);
    init_str_operand(&src, "src", false);
    init_str_operand(&dst, "dst", false);
    D_TEST_EXPR(clp_add_command_option(&root, &verbose) == D_OK);
    D_TEST_EXPR(clp_add_command_option(&root, &dry) == D_OK);
    D_TEST_EXPR(clp_add_command_operand(&root, &src) == D_OK);
    D_TEST_EXPR(clp_add_command_operand(&root, &dst) == D_OK);

    char *argv[] = {"prog", "src.txt", "-v", "dst.txt", "-n", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(verbose.value_set == true);
    D_TEST_EXPR(dry.value_set == true);
    D_TEST_EXPR(src.value_set == true);
    D_TEST_EXPR(dst.value_set == true);
    clp_cleanup(&root);
}

/* bool option can be set multiple times (not ARG_ACT_SET_UNIQUE) */
static void test_bool_option_set_twice_is_ok(void)
{
    Command root;
    Option verbose;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_bool_opt(&verbose, "verbose", "v", false);
    D_TEST_EXPR(clp_add_command_option(&root, &verbose) == D_OK);

    char *argv[] = {"prog", "-v", "--verbose", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(verbose.value_set == true);
    D_TEST_EXPR(verbose.value.value_bool == true);
    clp_cleanup(&root);
}

/* no-arg invocation with only optional options — parses cleanly */
static void test_no_args_with_optional_options(void)
{
    Command root;
    Option a, b;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_bool_opt(&a, "aaa", "a", false);
    init_bool_opt(&b, "bbb", "b", false);
    D_TEST_EXPR(clp_add_command_option(&root, &a) == D_OK);
    D_TEST_EXPR(clp_add_command_option(&root, &b) == D_OK);

    char *argv[] = {"prog", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(a.value_set == false);
    D_TEST_EXPR(b.value_set == false);
    clp_cleanup(&root);
}

/* long option with hyphen in name: --dry-run */
static void test_long_opt_with_hyphen_in_name(void)
{
    Command root;
    Option dry;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    init_bool_opt(&dry, "dry-run", "n", false);
    D_TEST_EXPR(clp_add_command_option(&root, &dry) == D_OK);

    char *argv[] = {"prog", "--dry-run", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(dry.value_set == true);
    clp_cleanup(&root);
}

/* ── nested subcommands ─────────────────────────────────────────────────── */

/* prog cmd subcmd — 2-level dispatch, returned command is innermost */
static void test_two_level_subcommand_dispatch(void)
{
    Command root, remote, add;
    D_TEST_EXPR(clp_init_command(&root,   0, "prog",   NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&remote, 1, "remote", NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&add,    2, "add",    NULL) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root,   &remote) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&remote, &add)    == D_OK);

    char *argv[] = {"prog", "remote", "add", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(cmd == &add);
    clp_cleanup(&root);
}

/* prog cmd subcmd subsubcmd — 3-level dispatch */
static void test_three_level_subcommand_dispatch(void)
{
    Command root, remote, add, origin;
    D_TEST_EXPR(clp_init_command(&root,   0, "prog",   NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&remote, 1, "remote", NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&add,    2, "add",    NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&origin, 3, "origin", NULL) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root,   &remote) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&remote, &add)    == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&add,    &origin) == D_OK);

    char *argv[] = {"prog", "remote", "add", "origin", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(cmd == &origin);
    clp_cleanup(&root);
}

/* parent_command pointer is set correctly for each level */
static void test_parent_command_pointer_chain(void)
{
    Command root, lvl1, lvl2;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&lvl1, 1, "cmd",  NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&lvl2, 2, "sub",  NULL) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &lvl1) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&lvl1, &lvl2) == D_OK);

    D_TEST_EXPR(lvl1.parent_command == &root);
    D_TEST_EXPR(lvl2.parent_command == &lvl1);
    clp_cleanup(&root);
}

/* root option before subcommand is parsed at root scope */
static void test_root_option_before_subcommand(void)
{
    Command root, push;
    Option verbose, force;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&push, 1, "push", NULL) == D_OK);
    init_bool_opt(&verbose, "verbose", "v", false);
    init_bool_opt(&force,   "force",   "f", false);
    D_TEST_EXPR(clp_add_command_option(&root, &verbose) == D_OK);
    D_TEST_EXPR(clp_add_command_option(&push, &force)   == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &push) == D_OK);

    char *argv[] = {"prog", "--verbose", "push", "--force", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(cmd == &push);
    D_TEST_EXPR(verbose.value_set == true);
    D_TEST_EXPR(force.value_set   == true);
    clp_cleanup(&root);
}

/* root option is unset when subcommand option is the only thing provided */
static void test_root_option_unset_when_only_sub_option_given(void)
{
    Command root, push;
    Option verbose, force;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&push, 1, "push", NULL) == D_OK);
    init_bool_opt(&verbose, "verbose", "v", false);
    init_bool_opt(&force,   "force",   "f", false);
    D_TEST_EXPR(clp_add_command_option(&root, &verbose) == D_OK);
    D_TEST_EXPR(clp_add_command_option(&push, &force)   == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &push) == D_OK);

    char *argv[] = {"prog", "push", "--force", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(verbose.value_set == false);
    D_TEST_EXPR(force.value_set   == true);
    clp_cleanup(&root);
}

/* options at all 3 levels each get correctly assigned */
static void test_options_at_each_of_three_levels(void)
{
    Command root, remote, add;
    Option verbose, url, force;
    Operand name;
    D_TEST_EXPR(clp_init_command(&root,   0, "prog",   NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&remote, 1, "remote", NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&add,    2, "add",    NULL) == D_OK);
    init_bool_opt(&verbose, "verbose", "v", false);
    init_str_opt(&url,      "url",     "u", false);
    init_bool_opt(&force,   "force",   "f", false);
    init_str_operand(&name, "name", false);
    D_TEST_EXPR(clp_add_command_option(&root,   &verbose) == D_OK);
    D_TEST_EXPR(clp_add_command_option(&remote, &url)     == D_OK);
    D_TEST_EXPR(clp_add_command_option(&add,    &force)   == D_OK);
    D_TEST_EXPR(clp_add_command_operand(&add,   &name)    == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root,   &remote) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&remote, &add)    == D_OK);

    char *argv[] = {"prog", "--verbose", "remote", "--url", "git@x.com", "add", "--force", "origin", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(cmd == &add);
    D_TEST_EXPR(verbose.value_set == true);
    D_TEST_EXPR(url.value_set     == true);
    D_TEST_EXPR(force.value_set   == true);
    D_TEST_EXPR(name.value_set    == true);
    D_TEST_STR_EQ(url.value.value_str,  "git@x.com");
    D_TEST_STR_EQ(name.value.value_str, "origin");
    clp_cleanup(&root);
}

/* sibling subcommands are isolated — parsing one doesn't affect the other */
static void test_sibling_subcommand_options_are_isolated(void)
{
    Command root, add, rm;
    Option force, recursive;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&add,  1, "add",  NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&rm,   2, "rm",   NULL) == D_OK);
    init_bool_opt(&force,     "force",     "f", false);
    init_bool_opt(&recursive, "recursive", "r", false);
    D_TEST_EXPR(clp_add_command_option(&add, &force)     == D_OK);
    D_TEST_EXPR(clp_add_command_option(&rm,  &recursive) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &add) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &rm)  == D_OK);

    char *argv[] = {"prog", "rm", "--recursive", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(cmd == &rm);
    D_TEST_EXPR(recursive.value_set == true);
    D_TEST_EXPR(force.value_set     == false);
    clp_cleanup(&root);
}

/* subcommand with both options and operands */
static void test_subcommand_with_options_and_operands(void)
{
    Command root, commit;
    Option amend;
    Operand msg;
    D_TEST_EXPR(clp_init_command(&root,   0, "prog",   NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&commit, 1, "commit", NULL) == D_OK);
    init_bool_opt(&amend, "amend", "a", false);
    init_str_operand(&msg, "message", false);
    D_TEST_EXPR(clp_add_command_option(&commit,  &amend) == D_OK);
    D_TEST_EXPR(clp_add_command_operand(&commit, &msg)   == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &commit) == D_OK);

    char *argv[] = {"prog", "commit", "--amend", "my message", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(cmd == &commit);
    D_TEST_EXPR(amend.value_set == true);
    D_TEST_EXPR(msg.value_set   == true);
    D_TEST_STR_EQ(msg.value.value_str, "my message");
    clp_cleanup(&root);
}

/* operand mode prevents subcommand dispatch */
static void test_operand_mode_prevents_subcommand_dispatch(void)
{
    Command root, push;
    Operand first, second;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&push, 1, "push", NULL) == D_OK);
    init_str_operand(&first,  "first",  false);
    init_str_operand(&second, "second", false);
    D_TEST_EXPR(clp_add_command_operand(&root, &first)  == D_OK);
    D_TEST_EXPR(clp_add_command_operand(&root, &second) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &push) == D_OK);

    /* "file.txt" triggers operand_mode; "push" is then treated as second operand */
    char *argv[] = {"prog", "file.txt", "push", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_NULL(cmd);
    D_TEST_EXPR(first.value_set  == true);
    D_TEST_EXPR(second.value_set == true);
    D_TEST_STR_EQ(first.value.value_str,  "file.txt");
    D_TEST_STR_EQ(second.value.value_str, "push");
    clp_cleanup(&root);
}

/* -- in subcommand context terminates its option parsing */
static void test_double_hyphen_in_subcommand_context(void)
{
    Command root, add;
    Option force;
    Operand file;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&add,  1, "add",  NULL) == D_OK);
    init_bool_opt(&force, "force", "f", false);
    init_str_operand(&file, "file", false);
    D_TEST_EXPR(clp_add_command_option(&add,  &force) == D_OK);
    D_TEST_EXPR(clp_add_command_operand(&add, &file)  == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &add) == D_OK);

    char *argv[] = {"prog", "add", "--", "--force", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(cmd == &add);
    D_TEST_EXPR(force.value_set == false);
    D_TEST_EXPR(file.value_set  == true);
    D_TEST_STR_EQ(file.value.value_str, "--force");
    clp_cleanup(&root);
}

/* combined short flags in subcommand context */
static void test_combined_short_flags_in_subcommand(void)
{
    Command root, add;
    Option recurse, force;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&add,  1, "add",  NULL) == D_OK);
    init_bool_opt(&recurse, "recursive", "r", false);
    init_bool_opt(&force,   "force",     "f", false);
    D_TEST_EXPR(clp_add_command_option(&add, &recurse) == D_OK);
    D_TEST_EXPR(clp_add_command_option(&add, &force)   == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &add) == D_OK);

    char *argv[] = {"prog", "add", "-rf", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(cmd == &add);
    D_TEST_EXPR(recurse.value_set == true);
    D_TEST_EXPR(force.value_set   == true);
    clp_cleanup(&root);
}

/* count option in subcommand context */
static void test_count_option_in_subcommand(void)
{
    Command root, run;
    Option verbose;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&run,  1, "run",  NULL) == D_OK);
    init_count_opt(&verbose, "verbose", "v");
    D_TEST_EXPR(clp_add_command_option(&run, &verbose) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &run) == D_OK);

    char *argv[] = {"prog", "run", "-vvv", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(cmd == &run);
    D_TEST_EXPR(verbose.value.value_usize == 3);
    clp_cleanup(&root);
}

/* list option in subcommand context */
static void test_list_option_in_subcommand(void)
{
    Command root, build;
    Option features;
    D_TEST_EXPR(clp_init_command(&root,  0, "prog",  NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&build, 1, "build", NULL) == D_OK);
    init_list_opt(&features, "features", "F");
    D_TEST_EXPR(clp_add_command_option(&build, &features) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &build) == D_OK);

    char *argv[] = {"prog", "build", "--features", "x,y,z", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(cmd == &build);
    D_TEST_EXPR(features.value_set == true);
    D_TEST_EXPR(d_dyn_array_get_size_safe(&features.value.value_list) == 3);
    clp_cleanup(&root);
}

/* long opt with = value in subcommand context */
static void test_long_opt_eq_value_in_subcommand(void)
{
    Command root, clone;
    Option depth;
    D_TEST_EXPR(clp_init_command(&root,  0, "prog",  NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&clone, 1, "clone", NULL) == D_OK);
    D_TEST_EXPR(clp_init_option_raw(&depth, "depth", "d", NULL, false,
                                    ARG_ACT_SET, (Value){0},
                                    TYPE_USIZE, false, false) == D_OK);
    D_TEST_EXPR(clp_add_command_option(&clone, &depth) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &clone) == D_OK);

    char *argv[] = {"prog", "clone", "--depth=1", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(cmd == &clone);
    D_TEST_EXPR(depth.value_set == true);
    D_TEST_EXPR(depth.value.value_usize == 1);
    clp_cleanup(&root);
}

/* subcommand with multiple operands, all set */
static void test_subcommand_with_multiple_operands(void)
{
    Command root, cp;
    Operand src, dst;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&cp,   1, "cp",   NULL) == D_OK);
    init_str_operand(&src, "src", false);
    init_str_operand(&dst, "dst", false);
    D_TEST_EXPR(clp_add_command_operand(&cp, &src) == D_OK);
    D_TEST_EXPR(clp_add_command_operand(&cp, &dst) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &cp) == D_OK);

    char *argv[] = {"prog", "cp", "a.txt", "b.txt", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(cmd == &cp);
    D_TEST_EXPR(src.value_set == true);
    D_TEST_EXPR(dst.value_set == true);
    D_TEST_STR_EQ(src.value.value_str, "a.txt");
    D_TEST_STR_EQ(dst.value.value_str, "b.txt");
    clp_cleanup(&root);
}

/* three sibling subcommands, third one dispatched */
static void test_three_siblings_dispatch_third(void)
{
    Command root, add, rm, ls;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&add,  1, "add",  NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&rm,   2, "rm",   NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&ls,   3, "ls",   NULL) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &add) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &rm)  == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &ls)  == D_OK);

    char *argv[] = {"prog", "ls", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(cmd == &ls);
    clp_cleanup(&root);
}

/* subcommand code is correct for identification */
static void test_subcommand_code_is_correct(void)
{
    Command root, add, rm;
    D_TEST_EXPR(clp_init_command(&root, 0,  "prog", NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&add,  42, "add",  NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&rm,   99, "rm",   NULL) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &add) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &rm)  == D_OK);

    char *argv[] = {"prog", "rm", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(cmd->code == 99);
    clp_cleanup(&root);
}

/* subcommand with inline short value and its own operand */
static void test_subcommand_short_inline_value_and_operand(void)
{
    Command root, push;
    Option remote;
    Operand branch;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&push, 1, "push", NULL) == D_OK);
    init_str_opt(&remote, "remote", "r", false);
    init_str_operand(&branch, "branch", false);
    D_TEST_EXPR(clp_add_command_option(&push,  &remote) == D_OK);
    D_TEST_EXPR(clp_add_command_operand(&push, &branch) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &push) == D_OK);

    char *argv[] = {"prog", "push", "-rorigin", "main", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(cmd == &push);
    D_TEST_EXPR(remote.value_set == true);
    D_TEST_EXPR(branch.value_set == true);
    D_TEST_STR_EQ(remote.value.value_str, "origin");
    D_TEST_STR_EQ(branch.value.value_str, "main");
    clp_cleanup(&root);
}

/* deep 3-level: options at level 1 and 3, not level 2 */
static void test_three_level_options_at_outer_and_inner(void)
{
    Command root, grp, cmd;
    Option quiet, debug;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&grp,  1, "grp",  NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&cmd,  2, "cmd",  NULL) == D_OK);
    init_bool_opt(&quiet, "quiet", "q", false);
    init_bool_opt(&debug, "debug", "d", false);
    D_TEST_EXPR(clp_add_command_option(&root, &quiet) == D_OK);
    D_TEST_EXPR(clp_add_command_option(&cmd,  &debug) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &grp) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&grp,  &cmd) == D_OK);

    char *argv[] = {"prog", "-q", "grp", "cmd", "--debug", NULL};
    Command *dispatched = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &dispatched) == D_OK);
    D_TEST_EXPR(dispatched == &cmd);
    D_TEST_EXPR(quiet.value_set == true);
    D_TEST_EXPR(debug.value_set == true);
    clp_cleanup(&root);
}

/* subcommand options don't bleed into sibling that was registered later */
static void test_sibling_registered_after_does_not_bleed(void)
{
    Command root, fetch, pull;
    Option all_fetch, rebase;
    D_TEST_EXPR(clp_init_command(&root,  0, "prog",  NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&fetch, 1, "fetch", NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&pull,  2, "pull",  NULL) == D_OK);
    init_bool_opt(&all_fetch, "all",    "a", false);
    init_bool_opt(&rebase,    "rebase", "r", false);
    D_TEST_EXPR(clp_add_command_option(&fetch, &all_fetch) == D_OK);
    D_TEST_EXPR(clp_add_command_option(&pull,  &rebase)    == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &fetch) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &pull)  == D_OK);

    char *argv[] = {"prog", "fetch", "--all", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(cmd == &fetch);
    D_TEST_EXPR(all_fetch.value_set == true);
    D_TEST_EXPR(rebase.value_set    == false);
    clp_cleanup(&root);
}

/* option before 2-level nested subcommand */
static void test_root_option_before_two_level_subcommand(void)
{
    Command root, remote, add;
    Option verbose, force;
    D_TEST_EXPR(clp_init_command(&root,   0, "prog",   NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&remote, 1, "remote", NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&add,    2, "add",    NULL) == D_OK);
    init_bool_opt(&verbose, "verbose", "v", false);
    init_bool_opt(&force,   "force",   "f", false);
    D_TEST_EXPR(clp_add_command_option(&root, &verbose) == D_OK);
    D_TEST_EXPR(clp_add_command_option(&add,  &force)   == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root,   &remote) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&remote, &add)    == D_OK);

    char *argv[] = {"prog", "-v", "remote", "add", "-f", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(cmd == &add);
    D_TEST_EXPR(verbose.value_set == true);
    D_TEST_EXPR(force.value_set   == true);
    clp_cleanup(&root);
}

/* no args to program with subcommands registered — command stays NULL */
static void test_no_args_with_subcommands_registered(void)
{
    Command root, add, rm;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&add,  1, "add",  NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&rm,   2, "rm",   NULL) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &add) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &rm)  == D_OK);

    char *argv[] = {"prog", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_NULL(cmd);
    clp_cleanup(&root);
}

/* subcommand with list operand consumes all trailing args */
static void test_subcommand_with_list_operand(void)
{
    Command root, add;
    Operand files;
    D_TEST_EXPR(clp_init_command(&root, 0, "prog", NULL) == D_OK);
    D_TEST_EXPR(clp_init_command(&add,  1, "add",  NULL) == D_OK);
    init_list_operand(&files, "files", false);
    D_TEST_EXPR(clp_add_command_operand(&add, &files) == D_OK);
    D_TEST_EXPR(clp_add_command_sub_command(&root, &add) == D_OK);

    char *argv[] = {"prog", "add", "a.c", "b.c", "c.c", NULL};
    Command *cmd = NULL;
    D_TEST_EXPR(clp_parse_args(&root, argv, &cmd) == D_OK);
    D_TEST_EXPR(cmd == &add);
    D_TEST_EXPR(files.value_set == true);
    D_TEST_EXPR(d_dyn_array_get_size_safe(&files.value.value_list) == 3);
    clp_cleanup(&root);
}

/* ── main ───────────────────────────────────────────────────────────────── */

int main(void)
{
    DTest tests[] = {
       
        D_TEST_GENERATE_TEST(test_list_option_in_subcommand),
    };
    D_TEST_RUN_TESTS(tests);
    return 0;
}
