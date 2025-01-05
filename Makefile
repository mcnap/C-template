CMP = gcc

# `-i' to format in-place
FMT = clang-format -i

# Use `FMT = true' to disable formatting.
# The `true' command is a shell built-in that discards its arguments and returns success.

# https://gcc.gnu.org/onlinedocs/gcc/Option-Summary.html
FLAG_SAN  = -fsanitize=address,undefined
FLAG_CMP  = $(FLAG_SAN) -g -MMD -pedantic -std=gnu99 -Wall -Werror
FLAG_LNK  = $(FLAG_SAN)

EXTN_HDR = .h
EXTN_SRC = .c
EXTN_DEP = .d
EXTN_OBJ = .o
EXTN_EXE = .exe

NAME_SRC = src
NAME_DST = dst
NAME_EXE = main

# ==============================================================================

PATH_SRC = $(wildcard $(NAME_SRC)/*$(EXTN_SRC))
PATH_DEP = $(patsubst $(NAME_SRC)/%$(EXTN_SRC), $(NAME_DST)/%$(EXTN_DEP), $(PATH_SRC))
PATH_OBJ = $(patsubst $(NAME_SRC)/%$(EXTN_SRC), $(NAME_DST)/%$(EXTN_OBJ), $(PATH_SRC))
PATH_EXE  = $(NAME_DST)/$(NAME_EXE)$(EXTN_EXE)

# A bare `make' command defaults to the first target.
all: $(PATH_EXE)

# To obtain the final executable, link the object files.
$(PATH_EXE): $(PATH_OBJ)
	$(CMP) $(PATH_OBJ) -o $(PATH_EXE) $(FLAG_LNK)

# To obtain an object file, compile the corresponding source code file.
$(NAME_DST)/%$(EXTN_OBJ): $(NAME_SRC)/%$(EXTN_SRC)
# `$@' = this target name      (here: the object file)
# `$<' = its first prerequsite (here: the source file)
	$(FMT) $<
	$(CMP) -c $< -o $@ $(FLAG_CMP)

# Note: Currently, the headers are not formatted!
-include $(PATH_DEP)

clean:
# "-f" to suppress "no such file" errors
	rm -f $(PATH_EXE) $(PATH_OBJ) $(PATH_DEP)

# Prevent unexpected behavior if files named `all' or `clean' exist.
.PHONY: all clean

# Disable built-in rules.
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:
