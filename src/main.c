#include "mylib.h"
#include <stdio.h>
#include <stdlib.h>

void fatal_usage(char const *const prog_name)
{
    fprintf(stderr, "USAGE: %s \n", prog_name);
    exit(EXIT_FAILURE);
}

int main(int const argc, char const *const *const argv)
{
    char const text[20] = "Hello, world!\n";
    inspect_char_buffer(text, sizeof text);

    return EXIT_SUCCESS;
}
