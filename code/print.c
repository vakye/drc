
static void print_use_stdout(void);
static void print_use_stderr(void);

static usize print_char(char c);
static usize print_newline(void);
static usize print(struct string msg);
static usize println(struct string msg);

static usize print_hex(usize value);
static usize print_usize(usize value);
static usize print_ssize(ssize value);

static usize print_f64(f64 value);

typedef usize print_write_out(void* data, usize size);

struct print_context {
    print_write_out* write_out;
};

struct print_context print_context =
{
    .write_out = &platform_write_stdout,
};

static void print_use_stdout(void)
{
    print_context.write_out = &platform_write_stdout;
}

static void print_use_stderr(void)
{
    print_context.write_out = &platform_write_stderr;
}

static usize print_char(char c)
{
    return print_context.write_out(&c, 1);
}

static usize print_newline(void)
{
    return print_char('\n');
}

static usize print(struct string msg)
{
    return print_context.write_out(msg.data, msg.size);
}

static usize println(struct string msg)
{
    usize written = 0;
    written += print(msg);
    written += print_newline();
    return (written);
}

static usize print_hex(usize value)
{
    char buffer[64] = {0};
    usize buffer_index = sizeof(buffer);
    usize buffer_count = 0;

    do
    {
        char nibble = (char)(value & 0xF);
        value >>= 4;

        char digit = (nibble < 10) ? (nibble + '0') : (nibble - 10 + 'a');

        buffer_index--;
        buffer_count++;

        buffer[buffer_index] = digit;
    } while (value);

    usize written = 0;
    written += print(str("0x"));
    written += print(str_data(buffer + buffer_index, buffer_count));
    return (written);
}

static usize print_usize(usize value)
{
    char buffer[64] = {0};
    usize buffer_index = sizeof(buffer);
    usize buffer_count = 0;

    do
    {
        char digit = '0' + (char)(value % 10);
        value /= 10;

        buffer_index--;
        buffer_count++;

        buffer[buffer_index] = digit;
    } while (value);

    usize written = print(str_data(buffer + buffer_index, buffer_count));
    return (written);
}

static usize print_ssize(ssize value)
{
    usize written = 0;

    if (value < 0)
    {
        written += print_char('-');
        value = -value;
    }

    written += print_usize(value);

    return (written);
}

static usize print_f64(f64 value)
{
    usize written = 0;

    // NOTE(vak): Very inaccurate and barebones floating point printing that doesn't
    // support special values (NaN, INF, ...)

    if (value < 0)
    {
        written += print_char('-');
        value = -value;
    }

    usize integer_part = (usize)value;
    f64   decimal_part = value - (f64)integer_part;

    written += print_usize(integer_part);
    written += print_char('.');

    for (usize index = 0; index < 3; index++)
    {
        decimal_part *= 10.0;
        usize digit = (usize)(decimal_part);
        decimal_part -= digit;

        written += print_usize(digit);
    }

    return (written);
}

