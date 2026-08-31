
#include "print.c"
#include "error.c"
#include "arena.c"
#include "lexer.c"
#include "parser.c"
#include "ir.c"

s32 main(void)
{
    f64 begin_ms = platform_ms_ticked();

    struct arena* arena = make_arena(mb(1), gb(64));
    struct string code = str("120 / 2*(10 + 10 - 5) % 7;");

    struct lexed_context lexed = {0};
    b32 tokenized_okay = tokenize_entire_str(arena, code, &lexed);
    if (tokenized_okay)
    {
        print_use_stdout();
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
        print_use_stdout();
        println(str("parser output: "));
        print_node(parsed.root_node);
    }
    else return (1);

    struct ir_inst_block inst_block = {0};
    b32 ir_okay = ir_generate(arena, &parsed, &inst_block);
    if (ir_okay)
    {
        print_use_stdout();
        print(str("ir output: (retval_id = "));
        print_ir_inst_id(inst_block.retval_id, 0);
        print(str(")"));
        print_newline();
        print_ir_inst_block(&inst_block);
    }

    delete_all_arenas();

    f64 end_ms = platform_ms_ticked();

    print_use_stdout();
    print(str("Finished in "));
    print_f64(end_ms - begin_ms);
    print(str("ms"));
    print_newline();

    return (0);
}

