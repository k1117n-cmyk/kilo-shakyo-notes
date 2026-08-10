# Kilo Tutorial Shakyo Notes

This repository contains my handwritten implementation of `kilo.c` while following [Build Your Own Text Editor](https://viewsourcecode.org/snaptoken/kilo/index.html) by snaptoken.

The purpose of this repository is learning. It is not an official translation or redistribution of the tutorial. The notes here are written as supplementary material for Japanese readers who want to follow the same tutorial and compare where they may get stuck.

## About The Tutorial

[Build Your Own Text Editor](https://viewsourcecode.org/snaptoken/kilo/index.html) is a step-by-step C tutorial that builds a small terminal text editor based on [antirez's kilo](https://github.com/antirez/kilo).

The tutorial is especially useful for learning:

- Raw terminal mode with `termios`
- Reading keypresses directly from the terminal
- ANSI escape sequences
- Screen redraw logic
- Dynamic row buffers
- File loading and saving
- Search
- Syntax highlighting

## Files

- `kilo.c`: the text editor implementation written while following the tutorial
- `makefile`: build rules for `kilo.c`
- `kilo_learning_notes.md`: Japanese learning notes, explanations, and common pitfalls
- `articles.md`: earlier notes about specific issues encountered during the tutorial

Some backup or experiment files may exist while learning. The main files to read are `kilo.c` and `kilo_learning_notes.md`.

## Build

This project uses a C compiler and `make`.

```sh
make
```

This builds an executable named `kilo`.

To open a file:

```sh
./kilo kilo.c
```

To remove the built executable:

```sh
make clean
```

The current build command is:

```sh
$(CC) kilo.c -o kilo -Wall -Wextra -pedantic -std=c99
```

Warnings are intentionally enabled because they are useful while learning C.

## Learning Notes

The main supplementary document is [kilo_learning_notes.md](./kilo_learning_notes.md).

It covers:

- How to read the original step-by-step diff format
- Why deleted lines can be easy to miss
- The difference between `void func()` and `void func(void)` in C
- A cursor bug caused by leaving an extra `\x1b[H`
- The easy-to-miss difference between `O` and `0` in escape sequence handling
- A high-level walkthrough of the finished `kilo.c`

## Common Pitfalls

While following the tutorial, I ran into a few issues that may be useful for other learners.

One important example was a cursor that stayed fixed in the upper-left corner. The direct cause was an old cursor-positioning escape sequence left in `editorRefreshScreen()`. The code compiled, but the runtime behavior was wrong.

Another example was the visual similarity between uppercase `O` and zero `0` in escape sequence parsing. This kind of typo does not always produce a compiler error, but it can make specific keys stop working.

These are documented in more detail in [kilo_learning_notes.md](./kilo_learning_notes.md).

## Attribution

This repository is based on learning from:

- Tutorial: [Build Your Own Text Editor](https://viewsourcecode.org/snaptoken/kilo/index.html) by snaptoken
- Original editor: [antirez's kilo](https://github.com/antirez/kilo)
- Tutorial appendix: [Appendices](https://viewsourcecode.org/snaptoken/kilo/08.appendices.html)

Please refer to the original tutorial and source repositories for their respective licenses. This repository's notes are intended as personal learning notes and supplementary commentary.
