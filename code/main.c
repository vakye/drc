
#include "print.c"
#include "error.c"
#include "arena.c"
#include "lexer.c"

s32 main(void)
{
    struct arena* arena = make_arena(mb(1), gb(64));
    struct string code = str("__hello_ + _world12_1h(arg0) 120 / 2*(10 + 10 - 5) % 7");

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

    delete_all_arenas();

    return (0);
}

