#!/bin/bash

if [ ! -d build ]; then
    mkdir -p build;
fi

SourceFile="code/linux.c"
OutputFile="build/drc"

CompileFlags=" \
    -g \
    -O0 \
    -ffreestanding \
    -fno-stack-protector \
    -fpie \
    -nostdlib \
    -Wall -Wextra -Wpedantic -Werror \
    -Wno-unused-parameter \
    -Wno-unused-variable \
    -Wno-unused-function \
    -Wno-switch \
    -o $OutputFile"

LinkFlags=" \
    -fuse-ld=lld \
    -Wl,-nostdlib \
    -Wl,-entry,EntryPoint"

clang $CompileFlags $LinkFlags $SourceFile

