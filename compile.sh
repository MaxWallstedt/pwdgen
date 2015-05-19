#!/bin/bash

CC="gcc"
CFLAGS="-pedantic -Wall -Wextra -Werror"
INPUT="pwdgen.c"
TARGET="pwdgen"

$CC $CFLAGS -o $TARGET $INPUT
