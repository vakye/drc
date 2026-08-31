
enum token_kind {
    TOKEN_KIND_EOF          = 0,
    TOKEN_KIND_INTEGER      = 1,
    TOKEN_KIND_IDENTIFIER   = 2,

    // NOTE(vak): Single character punctuation token kinds
    // are mapped directly to their respective ASCII codes.
};

struct token {
    enum token_kind kind;
    usize           from;
    usize           size;
    usize           line_index;
};

struct lexed_context {
    struct string   code;
    struct token*   tokens;
    usize           token_count;
};

static usize token_to_integer(struct lexed_context* lexed, struct token* tok);

static void error_at_line_prefix(usize line_index);
static void error_at_token(struct token* tok, struct string msg);

static void tokenize_digit  (struct string code, struct token* tok);
static void tokenize_ident  (struct string code, struct token* tok);
static void tokenize_punct  (struct string code, struct token* tok);

static b32 is_whitespace    (char c);
static b32 is_digit         (char c);
static b32 is_ident_start   (char c);
static b32 is_ident         (char c);
static b32 is_punct         (char c);

static b32 tokenize_entire_str(
    struct arena*           allocator,
    struct string           code,
    struct lexed_context*   result)
{
    if (code.size == 0)
        return (false);

    b32 okay = true;

    zero_type(result);

    result->code        = code;
    result->tokens      = arena_alloc_at(allocator);
    result->token_count = 0;

    usize line_index = 0;
    usize lex_at = 0;

    while (lex_at < code.size)
    {
        while (lex_at < code.size)
        {
            if (code.data[lex_at] == '\n')
                line_index++;

            if (!is_whitespace(code.data[lex_at]))
                break;

            lex_at++;
        }

        struct token current_tok = {0};

        current_tok.kind = TOKEN_KIND_EOF;
        current_tok.from = lex_at;
        current_tok.line_index = line_index;

        if (lex_at < code.size)
        {
            char c = code.data[lex_at];

            if (is_digit(c))                tokenize_digit(code, &current_tok);
            else if (is_ident_start(c))     tokenize_ident(code, &current_tok);
            else if (is_punct(c))           tokenize_punct(code, &current_tok);
            else
            {
                error_at_line_prefix(line_index);
                print               (str("unknown character utf("));
                print_hex           ((u8)c);
                print_char          (')');
                print_newline       ();

                okay = false;
                lex_at++;
            }

            lex_at += current_tok.size;
        }

        if (okay)
        {
            struct token* added_tok = arena_push_type(allocator, struct token);
            *added_tok = current_tok;

            result->token_count++;
        }
    }

    return (okay);
}

static usize token_to_integer(struct lexed_context* lexed, struct token* tok)
{
    struct string token_str = str_view(lexed->code, tok->from, tok->size);

    usize result = 0;
    for (usize index = 0; index < token_str.size; index++)
    {
        result *= 10;
        result += (token_str.data[index] - '0');
    }

    return (result);
}

static void error_at_line_prefix(usize line_index)
{
    print_use_stderr    ();
    print_error_label   ();
    print               (str("line "));
    print_usize         (line_index + 1); print_char(':');
    print_char          (' ');
}

static void error_at_token(struct token* tok, struct string msg)
{
    error_at_line_prefix(tok->line_index);
    println(msg);
}

static void tokenize_digit(struct string code, struct token* tok)
{
    tok->kind = TOKEN_KIND_INTEGER;
    usize lex_at = 1 + tok->from;

    while (lex_at < code.size)
    {
        if (!is_digit(code.data[lex_at]))
            break;

        lex_at++;
    }

    tok->size = lex_at - tok->from;
}

static void tokenize_ident(struct string code, struct token* tok)
{
    tok->kind = TOKEN_KIND_IDENTIFIER;
    usize lex_at = 1 + tok->from;

    while (lex_at < code.size)
    {
        if (!is_ident(code.data[lex_at]))
            break;

        lex_at++;
    }

    tok->size = lex_at - tok->from;
}

static void tokenize_punct(struct string code, struct token* tok)
{
    tok->kind = (enum token_kind)(code.data[tok->from]);
    tok->size = 1;
}

static b32 is_whitespace(char c)
{
    b32 result =
        (c == ' ') ||
        (c == '\t') ||
        (c == '\n') ||
        (c == '\r');

    return (result);
}

static b32 is_digit(char c)
{
    b32 result = (c >= '0') && (c <= '9');
    return (result);
}

static b32 is_ident_start(char c)
{
    b32 result =
        ((c >= 'a') && (c <= 'z')) ||
        ((c >= 'A') && (c <= 'Z')) ||
        ((c == '_'));

    return (result);
}

static b32 is_ident(char c)
{
    b32 result = is_ident_start(c) || is_digit(c);
    return (result);
}

static b32 is_punct(char c)
{
    b32 result =
        ((c >=  33) && (c <=  47))  ||
        ((c >=  58) && (c <=  63))  ||
        ((c >=  91) && (c <=  94))  ||
        ((c ==  96))                ||
        ((c >= 123) && (c <= 126));

    return (result);
}

