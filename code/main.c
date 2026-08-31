
#include "print.c"
#include "error.c"
#include "arena.c"
#include "lexer.c"
#include "parser.c"

s32 main(void)
{
    struct arena* arena = make_arena(mb(1), gb(64));
    struct string code = str("120 / 2*(10 + 10 - 5) % 7");

    struct lexed_context lexed = {0};
    b32 tokenized_okay = tokenize_entire_str(arena, code, &lexed);
    if (tokenized_okay)
    {
        println(str("tokenizer output: "));

        for (usize index = 0; index < lexed.token_count; index++)
        {
            struct token* tok = lexed.tokens + index;
            struct string token_str = str_view(code, tok->from, tok->size);
            print(str("    "));
            println(token_str);
        }
    }
    else return (1);

    struct parsed_context parsed = {0};
    b32 parsed_okay = parse_root(arena, &lexed, &parsed);
    if (parsed_okay)
    {
        println(str("parser output: "));
        print_node(parsed.root_node);
    }
    else return (1);

    delete_all_arenas();

    return (0);
}

