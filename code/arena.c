
#define MAX_ARENA_COUNT         (128)
#define ARENA_BLOCK_ALIGNMENT   (kb(256))

struct arena;

static struct arena*    make_arena(usize min_commited, usize min_reserved);
static void             delete_arena(struct arena* arena);
static void             reset_arena(struct arena* arena);
static void             delete_all_arenas(void);

static void*            arena_push_size(struct arena* arena, usize size);
static usize            arena_used(struct arena* arena);
static void*            arena_base_at(struct arena* arena);
static void*            arena_alloc_at(struct arena* arena);

#define arena_push_type(arena, type)        (type*)arena_push_size(arena, sizeof(type))
#define arena_push_arr(arena, type, count)  (type*)arena_push_size(arena, sizeof(type) * (count))

struct arena {
    void* base;
    usize used;
    usize commited;
    usize reserved;
};

struct arena list_of_arenas[MAX_ARENA_COUNT] = {0};

static struct arena* make_arena(usize min_commited, usize min_reserved)
{
    ensure(min_reserved > 0,                str("min_reserved=0 in make_arena()"));
    ensure(min_reserved >= min_commited,    str("min_commited is more than min_reserved in make_arena()"));

    struct arena* result = 0;
    for (usize index = 0; index < MAX_ARENA_COUNT; index++)
    {
        if (list_of_arenas[index].base == 0)
        {
            result = list_of_arenas + index;
            break;
        }
    }

    ensure(result, str("no more arena slots available for make_arena() to use."));

    zero_type(result);

    result->commited = align_up(min_commited, ARENA_BLOCK_ALIGNMENT);
    result->reserved = align_up(min_reserved, ARENA_BLOCK_ALIGNMENT);

    result->base = platform_reserve_mem(result->reserved);
    ensure(result->base, str("failed to reserve memory for arena in make_arena()"));

    b32 commit_okay = platform_commit_mem(result->base, result->commited);
    ensure(commit_okay, str("failed to commit memory for arena in make_arena()"));

    return (result);
}

static void delete_arena(struct arena* arena)
{
    if (arena->base)
        platform_release_mem(arena->base, arena->reserved);

    zero_type(arena);
}

static void reset_arena(struct arena* arena)
{
    arena->used = 0;
}

static void delete_all_arenas(void)
{
    for (usize index = 0; index < MAX_ARENA_COUNT; index++)
        delete_arena(list_of_arenas + index);
}

static void* arena_push_size(struct arena* arena, usize size)
{
    if (arena->used + size > arena->commited)
    {
        usize expand_size = (arena->used + size) - arena->commited;
        usize commit_size = align_up(expand_size, ARENA_BLOCK_ALIGNMENT);
        void* commit_at   = (u8*)arena->base + arena->commited;

        b32 commit_okay = platform_commit_mem(commit_at, commit_size);
        ensure(commit_okay, str("failed to commit memory memory for arena in arena_push_size()."));

        arena->commited += commit_size;
    }

    void* result = (u8*)arena->base + arena->used;
    arena->used += size;

    return (result);
}

static usize arena_used(struct arena* arena)
{
    return (arena->used);
}

static void* arena_base_at(struct arena* arena)
{
    return (arena->base);
}

static void* arena_alloc_at(struct arena* arena)
{
    return ((u8*)arena->base + arena->used);
}

