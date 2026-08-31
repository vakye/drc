
static usize platform_write_stdout(void* data, usize size);
static usize platform_write_stderr(void* data, usize size);
static void* platform_reserve_mem(usize size);
static b32   platform_commit_mem(void* mem, usize size);
static void  platform_release_mem(void* mem, usize reserved_size);
static void  platform_exit(u8 code);

