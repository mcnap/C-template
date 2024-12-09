#define _GNU_SOURCE // `TEMP_FAILURE_RETRY'
#include <ctype.h> // isspace
#include <errno.h> // errno
#include <limits.h> // INT_MAX, LONG_MAX
#include <signal.h> // sigaction, SIGKILL
#include <stdio.h> // fprintf, perror, ...
#include <stdlib.h> // exit, strtol
#include <string.h> // memset
#include <sys/wait.h> // waitpid
#include <unistd.h> // read, write

// Use the `FATAL' macro for convenience.
void fatal(
    char const *const func_name,
    char const *const file_name,
    int const line_number)
{
    // If it weren't for this check, `perror' would print a misleading "Success".
    if (0 == errno)
        fprintf(stderr, "%s: Unknown error\n", func_name);
    else
        perror(func_name);

    // Try to kill all processes whose process group ID is equal to ours.
    kill(0, SIGKILL);

    fprintf(stderr, "%s:%d\n", file_name, line_number);
    exit(EXIT_FAILURE);
}

#define FATAL(func_name) fatal(func_name, __FILE__, __LINE__);

// Meant for debugging. Prints to `stderr' to circumvent buffering.
void inspect_char_buffer(
    char const *buf,
    size_t const size)
{
    if (NULL == buf)
    {
        fputs("Cannot visualize the buffer. Null pointer received.\n", stderr);
        return;
    }

    // Fake a formatting operation to get the number of digits.
    int iw = snprintf(NULL, 0, "%zu", size);
    if (iw < 0)
        FATAL("snprintf");

    size_t i = 0;
    char const *p = buf;
    for (; i < size; ++i, ++p)
    {
        fprintf(stderr, "%*zu ", iw, i);
        if (0 == *p)
            fputs("     ", stderr);
        else if ('\n' == *p)
            fputs("+---+", stderr);
        else
            fprintf(stderr, "| %c |", *p);
        fprintf(stderr, " %3i\n", (int)(*p));
    }
}

// Credits:
// Ciro Santilli : https://stackoverflow.com/a/12923949
// Dan Moulding  : https://stackoverflow.com/a/6154614
typedef enum
{
    STR2INT_SUCCESS,
    STR2INT_OVERFLOW,
    STR2INT_UNDERFLOW,
    STR2INT_INCONVERTIBLE,
} str2int_errno;

str2int_errno str2int(int *dst, char const *src, unsigned short const base)
{
    char *after_num;

    if (src == NULL || src[0] == '\0' || isspace(src[0]))
    {
        errno = EINVAL;
        return STR2INT_INCONVERTIBLE;
    }

    errno = 0;
    long l = strtol(src, &after_num, base);

    if (l < INT_MIN || (errno == ERANGE && l == LONG_MIN))
        return STR2INT_UNDERFLOW;
    if (l > INT_MAX || (errno == ERANGE && l == LONG_MAX))
        return STR2INT_OVERFLOW;
    if (*after_num != '\0')
    {
        errno = EINVAL;
        return STR2INT_INCONVERTIBLE;
    }

    *dst = l;
    return STR2INT_SUCCESS;
}

// Based on: https://sop.mini.pw.edu.pl/pl/sop1/lab/l1/
// Usage:
// ssize_t num_read;
// if (0 > (num_read = bulk_read(fd, buf, count)))
//     FATAL("bulk_read");
ssize_t bulk_read(int const fd, char *dst, size_t count)
{
    ssize_t curr, total = 0;
    do
    {
        curr = TEMP_FAILURE_RETRY(read(fd, dst, count));
        if (curr < 0) // error (more serious that `EINTR')
            return curr;
        if (0 == curr) // EOF
            return total;
        total += curr;
        count -= curr;
        dst += curr;
    } while (count > 0);
    return total;
}

ssize_t bulk_write(int const fd, char const *src, size_t count)
{
    ssize_t curr, total = 0;
    do
    {
        curr = TEMP_FAILURE_RETRY(write(fd, src, count));
        if (curr < 0) // error (more serious that `EINTR')
            return curr;
        total += curr;
        count -= curr;
        src += curr;
    } while (count > 0);
    return total;
}

void print_pid(void)
{
    if (0 > printf("[%i] ", getpid()))
        FATAL("printf");
}

void handler_sigchld(int const sig)
{
    // BEHAVIOR OF `waitpid' ===================================================
    //
    // Child process available?
    // => Return its PID.
    //
    // Child process exists, but not yet available?
    // => If `WNOHANG' was passed, return 0 immediately.
    //    Otherwise, block the current thread until a child becomes available.
    //
    // No unwaited child process exists at all?
    // => Set `errno' to `ECHILD'.
    // => Return -1.
    //
    // Other error?
    // => Set `errno' appropriately.
    //    (In particular `EINTR' if the call was interrupted by a signal.)
    // => Return -1.
    
    pid_t pid;
    for (;;)
    {
        pid = waitpid(-1, NULL, WNOHANG);
        if (pid > 0)
            continue;
        else if (0 == pid)
            return;
        else if (ECHILD == errno)
            return;
        else
            FATAL("waitpid");
    }
}

void set_signal_handler(
    int const signal_kind,
    void (* const new_handler)(int))
{
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = new_handler;
    
    if (sigaction(signal_kind, &act, NULL))
        FATAL("sigaction");
}
