# On x86-64 Linux, syscall number 1 is write, handle 1 is stdout

__syscall(1, 1, "Hello world\n", 12)
