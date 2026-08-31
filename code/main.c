
s32 main(void)
{
    char message[] = "hello, world!\n";

    platform_write_stdout(message, sizeof(message) - 1);

    return (0);
}

