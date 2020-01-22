# Perf

This is a simplified profiler based on `strace`
that counts syscall times and reports a summary on the program's exit.

## Build
    
    make

## Usage

    ./perf COMMAND [ARG]...

## Example

If you enter
    
    ./perf ls

in your terminal, the result may be

    syscall         % time
    --------------- ------
    read             27.41
    mmap             15.22
    mprotect         13.03
    open              8.11
    fstat             6.73
    close             5.84
    access            4.83
    write             3.24
    execve            2.77
    getdents          2.73
    statfs            2.10
    brk               1.35
    set_tid_address   1.35
    ioctl             1.22
    getrlimit         1.05
    rt_sigaction      1.05
    arch_prctl        0.67
    set_robust_list   0.55
    rt_sigprocmask    0.50
    munmap            0.25
