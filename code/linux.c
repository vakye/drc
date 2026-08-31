
#include "common.c"
#include "platform.c"
#include "main.c"

__attribute__((force_align_arg_pointer))
void EntryPoint(void)
{
    s32 code = main();
    platform_exit(code);
}

enum linux_syscall_nr {
    LINUX_SYSCALL_NR_WRITE              = 1,
    LINUX_SYSCALL_NR_MMAP               = 9,
    LINUX_SYSCALL_NR_MPROTECT           = 10,
    LINUX_SYSCALL_NR_MUNMAP             = 11,
    LINUX_SYSCALL_NR_EXIT               = 60,
    LINUX_SYSCALL_NR_CLOCK_GETTIME      = 228,
};

#define STDOUT_FILENO (1)
#define STDERR_FILENO (2)

#define PROT_NONE   (0x00)
#define PROT_READ   (0x01)
#define PROT_WRITE  (0x02)
#define PROT_EXEC   (0x04)

#define MAP_PRIVATE     (0x02)
#define MAP_ANONYMOUS   (0x20)

#define CLOCK_MONOTONIC (1)

static usize linux_syscall(
    enum linux_syscall_nr syscall_nr,
    usize arg0, usize arg1, usize arg2,
    usize arg3, usize arg4, usize arg5
)
{
    usize result = 0;

    register usize r10 __asm__("r10") = arg3;
    register usize r8  __asm__("r8")  = arg4;
    register usize r9  __asm__("r9")  = arg5;

    __asm__ volatile (
        "syscall" :
        "=a"(result) :
        "a"(syscall_nr),
        "D"(arg0),
        "S"(arg1),
        "d"(arg2),
        "r"(r10),
        "r"(r8),
        "r"(r9) :
        "memory", "rcx", "r11");

    return (result);
}

#define write(fd, data, size) \
    (ssize)linux_syscall(LINUX_SYSCALL_NR_WRITE, fd, (usize)data, size, 0, 0, 0)

#define mmap(addr, size, prot_flags, map_flags, fd, offset) \
    (void*)linux_syscall(LINUX_SYSCALL_NR_MMAP, (usize)(addr), size, prot_flags, map_flags, fd, offset)

#define mprotect(addr, size, prot_flags) \
    (ssize)linux_syscall(LINUX_SYSCALL_NR_MPROTECT, (usize)(addr), size, prot_flags, 0, 0, 0)

#define munmap(addr, size) \
    (ssize)linux_syscall(LINUX_SYSCALL_NR_MUNMAP, (usize)(addr), size, 0, 0, 0, 0)

#define exit(code) \
    linux_syscall(LINUX_SYSCALL_NR_EXIT, code, 0, 0, 0, 0, 0)

#define clock_gettime(clockid, tp) \
    linux_syscall(LINUX_SYSCALL_NR_CLOCK_GETTIME, clockid, (usize)(tp), 0, 0, 0, 0)

struct linux_timespec {
    u64 secs;
    u64 nsecs;
};

static f64 platform_ms_ticked(void)
{
    struct linux_timespec now = {0};
    clock_gettime(CLOCK_MONOTONIC, &now);

    // TODO(vak): This might be pretty inaccurate based on how long the system has been
    // running for... It might be better to store the raw 128-bit time value, and then
    // use another function to get the elapsed time to avoid floating point inaccuracies.

    f64 system_ms_ticked = (now.secs*1000.0) + (now.nsecs * 1e-6);
    return (system_ms_ticked);
}

static usize platform_write_stdout(void* data, usize size)
{
    ssize write_result = write(STDOUT_FILENO, data, size);
    usize written = (write_result > 0) ? (write_result) : (0);
    return (written);
}

static usize platform_write_stderr(void* data, usize size)
{
    ssize write_result = write(STDERR_FILENO, data, size);
    usize written = (write_result > 0) ? (write_result) : (0);
    return (written);
}

static void* platform_reserve_mem(usize size)
{
    ssize map_result = (ssize)mmap(0, size, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);

    void* result = 0;
    if (map_result > 0)
        result = (void*)map_result;

    return (result);
}

static b32 platform_commit_mem(void* mem, usize size)
{
    ssize commit_result = mprotect(mem, size, PROT_READ|PROT_WRITE);

    b32 okay = (commit_result >= 0);
    return (okay);
}

static void platform_release_mem(void* mem, usize reserved_size)
{
    if (mem && reserved_size)
        munmap(mem, reserved_size);
}

static void platform_exit(u8 code)
{
    exit(code);
}

