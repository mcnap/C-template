#define _GNU_SOURCE // `TEMP_FAILURE_RETRY'
#include <ctype.h> // isspace
#include <errno.h> // errno
#include <limits.h> // INT_MAX, LONG_MAX
#include <inttypes.h> // strtoumax, uintmax_t
#include <signal.h> // sigaction, SIGKILL
#include <stdio.h> // fprintf, perror, ...
#include <stdlib.h> // exit
#include <string.h> // memset
#include <sys/wait.h> // waitpid
#include <time.h> // nanosleep
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

    fprintf(stderr, "%s:%d\n", file_name, line_number);
    
    // Die, along with your children.
    kill(0, SIGKILL);
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
    STR2UNUM_SUCCESS,
    STR2UNUM_OVERFLOW,
    STR2UNUM_UNDERFLOW,
    STR2UNUM_INCONVERTIBLE,
} str2unum_errno;

str2unum_errno str2unum(uintmax_t *dst, char const *src, uintmax_t my_min, uintmax_t my_max)
{
    char *after_num;
    uintmax_t parsed;

    if (src == NULL || src[0] == '\0' || isspace(src[0]))
    {
        errno = EINVAL;
        return STR2UNUM_INCONVERTIBLE;
    }

    errno = 0;
    parsed = strtoumax(src, &after_num, 10);

    if (*after_num != '\0')
        errno = EINVAL;
    if (errno == EINVAL)
        return STR2UNUM_INCONVERTIBLE;

    if (parsed < my_min || parsed > my_max)
        errno = ERANGE;
    if (errno == ERANGE)
        return STR2UNUM_OVERFLOW;

    *dst = parsed;
    return STR2UNUM_SUCCESS;
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

void send_signal(pid_t const pid, int const sig)
{
    if (-1 == pid)
    {
        fprintf(stderr, "Sending a signal everywhere is a bad idea! "
        "If you meant to send the signal to each member of your process group, "
        "pass `0' as the PID rather than `-1'.");
        exit(EXIT_FAILURE);
    }
    
    if (kill(pid, sig))
        FATAL("kill");
}

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
void handler_sigchld(int const sig)
{   
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

void uninterrupted_millisleep(unsigned const ms)
{
    unsigned const s = ms / 1000;
    unsigned const ns = (ms % 1000) * 1000000;

    struct timespec t = {s, ns};
    for (;;)
    {
        errno = 0;
        if (0 == nanosleep(&t, &t))
            break; // finished
        if (EINTR == errno)
            continue; // interrupted
        FATAL("nanosleep");
        // invalid argument (or any other error)
    }
}
