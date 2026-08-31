
#define ensure(expression, error_msg) \
    if (!(expression)) \
        panic_full(str(__FILE__), __LINE__, error_msg)

#define panic(error_msg) panic_full(str(__FILE__), __LINE__, error_msg)

static void print_error_label(void)
{
    print(str("[\033[31merror\033[0m]: "));
}

static void panic_full(struct string file_path, usize line_num, struct string msg)
{
    print_use_stderr    ();
    print_error_label   ();
    print               (file_path); print_char(':');
    print_usize         (line_num);  print_char(':');
    print_char          (' ');
    print               (msg);
    print_newline       ();
    platform_exit       (1);
}

