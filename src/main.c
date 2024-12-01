#include "mylib.h"
#include <stdio.h>
#include <stdlib.h>

int main(int const argc, char const *const *const argv)
{
    char const text[20] = "Hello, world!\n";
    inspect_char_buffer(text, sizeof(text));

    return EXIT_SUCCESS;
}
