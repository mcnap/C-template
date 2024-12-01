CMP = gcc
FMT = clang-format -i

# https://gcc.gnu.org/onlinedocs/gcc/Option-Summary.html
SAN_FLAGS  = -fsanitize=address,undefined
CMP_FLAGS  = $(SAN_FLAGS) -g -pedantic -Wall -Werror
LNK_FLAGS  = $(SAN_FLAGS)
FMT_ENABLE = 1

SRC_DIR   = src
DST_DIR   = dst
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(DST_DIR)/%.o, $(SRC_FILES))
EXE_FILE  = $(DST_DIR)/main

# ==============================================================================

# A bare `make' command defaults to the first target.
all: $(EXE_FILE)

$(EXE_FILE): $(OBJ_FILES)
	$(CMP) -o $(EXE_FILE) $(OBJ_FILES) $(LNK_FLAGS)

$(DST_DIR)/%.o: $(SRC_DIR)/%.c
ifeq ($(FMT_ENABLE), 1)
# `-i' to format in-place
	clang-format -i $<
endif
# `$@' = this target name      (here: the object file)
# `$<' = its first prerequsite (here: the source file)
	$(CMP) -o $@ -c $< $(CMP_FLAGS)

clean:
# "-f" to suppress "no such file" errors
	rm -f $(EXE_FILE) $(OBJ_FILES)

# Suppose our working directory contains a file inconviently named `clean'.
# If we ran `make clean', Make would be tricked into thinking
# that our request has already been handled.
# Below is a solution to this problem.
# "Phony" targets are always considered out-of-date.
.PHONY: all clean