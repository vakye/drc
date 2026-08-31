
enum node_kind {
    NODE_KIND_NIL           = 0,
    NODE_KIND_INTEGER,
    NODE_KIND_ADD,
    NODE_KIND_SUB,
    NODE_KIND_MUL,
    NODE_KIND_DIV,
    NODE_KIND_MOD
};

struct node {
    enum node_kind kind;
    struct token* token;

    union
    {
        struct { usize integer; };
        struct {
            struct node* left;
            struct node* right;
        };
    };
};

struct parsed_context {
    struct node*    root_node;
    usize           token_index;
};

static b32 parse_stmt       (struct arena* allocator, struct lexed_context* lexed, struct parsed_context* parsed, struct node** result);
static b32 parse_expr       (struct arena* allocator, struct lexed_context* lexed, struct parsed_context* parsed, struct node** result);
static b32 parse_sum        (struct arena* allocator, struct lexed_context* lexed, struct parsed_context* parsed, struct node** result);
static b32 parse_factor     (struct arena* allocator, struct lexed_context* lexed, struct parsed_context* parsed, struct node** result);
static b32 parse_primary    (struct arena* allocator, struct lexed_context* lexed, struct parsed_context* parsed, struct node** result);

static struct token* parse_cur_tok   (struct lexed_context* lexed, struct parsed_context* parsed);
static void parse_next_tok           (struct lexed_context* lexed, struct parsed_context* parsed);
static void parse_error_here         (struct lexed_context* lexed, struct parsed_context* parsed, struct string msg);

static struct node*     add_node(struct arena* allocator, enum node_kind kind, struct token* tok);
static void             print_node(struct node* node);

static b32 parse_root(struct arena* allocator, struct lexed_context* lexed, struct parsed_context*  parsed)
{
    b32 parse_okay = parse_stmt(allocator, lexed, parsed, &parsed->root_node);
    return (parse_okay);
}

static b32 parse_stmt(struct arena* allocator, struct lexed_context* lexed, struct parsed_context* parsed, struct node** result)
{
    b32 parse_okay = parse_expr(allocator, lexed, parsed, result);

    if (parse_cur_tok(lexed, parsed)->kind != ';')
    {
        parse_error_here(lexed, parsed, str("expected ';' at end of statement"));
        parse_okay = false;
    }
    else parse_next_tok(lexed, parsed);

    return (parse_okay);
}

static b32 parse_expr(struct arena* allocator, struct lexed_context* lexed, struct parsed_context* parsed, struct node** result)
{
    b32 parse_okay = parse_sum(allocator, lexed, parsed, result);
    return (parse_okay);
}

static b32 parse_sum(struct arena* allocator, struct lexed_context* lexed, struct parsed_context* parsed, struct node** result)
{
    b32 parse_okay = parse_factor(allocator, lexed, parsed, result);

    for (;;)
    {
        enum node_kind node_kind = NODE_KIND_NIL;
        struct token* tok = parse_cur_tok(lexed, parsed);

        if      (tok->kind == '+') node_kind = NODE_KIND_ADD;
        else if (tok->kind == '-') node_kind = NODE_KIND_SUB;

        if (node_kind != NODE_KIND_NIL)
        {
            parse_next_tok(lexed, parsed);

            struct node* right = 0;
            parse_okay &= parse_factor(allocator, lexed, parsed, &right);

            struct node* op_node = add_node(allocator, node_kind, tok);
            op_node->left = *result;
            op_node->right = right;

            *result = op_node;
        }
        else break;
    }

    return (parse_okay);
}

static b32 parse_factor(struct arena* allocator, struct lexed_context* lexed, struct parsed_context* parsed, struct node** result)
{
    b32 parse_okay = parse_primary(allocator, lexed, parsed, result);

    for (;;)
    {
        enum node_kind node_kind = NODE_KIND_NIL;
        struct token* tok = parse_cur_tok(lexed, parsed);

        if      (tok->kind == '*') node_kind = NODE_KIND_MUL;
        else if (tok->kind == '/') node_kind = NODE_KIND_DIV;
        else if (tok->kind == '%') node_kind = NODE_KIND_MOD;

        if (node_kind != NODE_KIND_NIL)
        {
            parse_next_tok(lexed, parsed);

            struct node* right = 0;
            parse_okay &= parse_primary(allocator, lexed, parsed, &right);

            struct node* op_node = add_node(allocator, node_kind, tok);
            op_node->left = *result;
            op_node->right = right;

            *result = op_node;
        }
        else break;
    }

    return (parse_okay);
}

static b32 parse_primary(struct arena* allocator, struct lexed_context* lexed, struct parsed_context* parsed, struct node** result)
{
    b32 parse_okay = true;

    struct token* tok = parse_cur_tok(lexed, parsed);

    if (tok->kind == TOKEN_KIND_INTEGER)
    {
        struct node* node = add_node(allocator, NODE_KIND_INTEGER, tok);
        node->integer = token_to_integer(lexed, tok);
        *result = node;

        parse_next_tok(lexed, parsed);
    }
    else if (tok->kind == '(')
    {
        parse_next_tok(lexed, parsed);
        parse_okay &= parse_expr(allocator, lexed, parsed, result);

        if (parse_cur_tok(lexed, parsed)->kind != ')')
        {
            parse_error_here(lexed, parsed, str("expected matching ')' in expression"));
            parse_okay = false;
        }
        else parse_next_tok(lexed, parsed);
    }
    else if (tok->kind == ';')
    {
        // NOTE(vak): ignore
    }
    else
    {
        parse_error_here(lexed, parsed, str("syntax error"));
        parse_okay = false;
    }

    return (parse_okay);
}

static struct token* parse_cur_tok(struct lexed_context* lexed, struct parsed_context* parsed)
{
    struct token* token = lexed->tokens + parsed->token_index;
    return (token);
}

static void parse_next_tok(struct lexed_context* lexed, struct parsed_context* parsed)
{
    if (parsed->token_index < lexed->token_count)
        parsed->token_index++;
}

static void parse_error_here(struct lexed_context* lexed, struct parsed_context* parsed, struct string msg)
{
    error_at_token(lexed->tokens + parsed->token_index, msg);
}

static struct node* add_node(struct arena* allocator, enum node_kind kind, struct token* tok)
{
    struct node* node = arena_push_type(allocator, struct node);
    zero_type(node);

    node->kind = kind;
    node->token = tok;

    return (node);
}

static void print_node(struct node* node)
{
    static struct string node_kind_names[] =
    {
        #define do_node_kind_name(kind) \
            [NODE_KIND_##kind] = {#kind, sizeof(#kind) - 1}

        do_node_kind_name(NIL),
        do_node_kind_name(INTEGER),
        do_node_kind_name(ADD),
        do_node_kind_name(SUB),
        do_node_kind_name(MUL),
        do_node_kind_name(DIV),
        do_node_kind_name(MOD),

        #undef do_node_kind_name
    };

    static usize depth = 0;
    depth++;

    for (usize index = 0; index < depth; index++)
        print(str("    "));

    print(node_kind_names[node->kind]);
    print(str(": "));

    switch (node->kind)
    {
        default: print_newline(); break;

        case NODE_KIND_INTEGER:
        {
            print_usize(node->integer);
            print_newline();
        } break;

        case NODE_KIND_ADD:
        case NODE_KIND_SUB:
        case NODE_KIND_MUL:
        case NODE_KIND_DIV:
        case NODE_KIND_MOD:
        {
            print_newline();
            print_node(node->left);
            print_node(node->right);
        } break;
    }

    depth--;
}

