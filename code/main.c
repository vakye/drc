
#include "print.c"
#include "error.c"
#include "arena.c"

s32 main(void)
{
    struct arena* my_arena = make_arena(mb(1), gb(64));

    usize* stuff0 = arena_push_arr(my_arena, usize, 1024);
    usize* stuff1 = arena_push_arr(my_arena, usize, 4096);

    print(str("stuff0 = "));
    print_hex((usize)stuff0);
    print_newline();

    print(str("stuff1 = "));
    print_hex((usize)stuff1);
    print_newline();

    println(str("my_arena:"));
    print(str("    base=")); print_hex((usize)arena_base_at(my_arena)); print_newline();
    print(str("    used=")); print_hex((usize)arena_used(my_arena)); print_newline();

    println(str("hello, world!"));

    delete_all_arenas();

    return (0);
}

