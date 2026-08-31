
typedef signed char         s8;
typedef signed short        s16;
typedef signed int          s32;
typedef signed long long    s64;

typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;

typedef s64 ssize;
typedef u64 usize;

typedef float f32;
typedef double f64;

typedef u8 b8;
typedef u32 b32;

#define true (1)
#define false (0)

#define U32_MAX ((u32)(0xFFFFFFFF))

#define kb(amount) ((ssize)(amount) << 10)
#define mb(amount) ((ssize)(amount) << 20)
#define gb(amount) ((ssize)(amount) << 30)
#define tb(amount) ((ssize)(amount) << 40)

#define align_up(value, power_of_2) (((value) + (power_of_2) - 1) & ~((power_of_2) - 1))

void* memset(void* dest_init, s32 byte, usize size)
{
    u8* dest = (u8*)dest_init;
    while (size--)
        *dest++ = (u8)byte;

    return (dest_init);
}

void* memcpy(void* dest_init, void* source_init, usize size)
{
    u8* dest = (u8*)dest_init;
    u8* source = (u8*)source_init;

    while (size--)
        *dest++ = *source++;

    return (dest_init);
}

#define zero_type(pointer)          zero_mem(pointer, sizeof(*(pointer)))
#define zero_arr(pointer, count)    zero_mem(pointer, sizeof(*(pointer)) * (count))

static void zero_mem(void* dest_init, usize size)                       { memset(dest_init, 0, size); }
static void fill_mem(void* dest_init, u8 byte, usize size)              { memset(dest_init, byte, size); }
static void copy_mem(void* dest_init, void* source_init, usize size)    { memcpy(dest_init, source_init, size); }

struct string {
    char* data;
    usize size;
};

#define str(literal)            (struct string){literal, sizeof(literal) - 1}
#define str_data(data, size)    (struct string){data, size}

static struct string str_view(struct string source, usize from, usize size)
{
    struct string result = str_data(source.data + from, size);
    return (result);
}
