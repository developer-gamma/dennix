/* Copyright (c) 2018 Dennis Wölfing
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* sh/parser.h
 * Shell parser.
 */

#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <stddef.h>

struct Redirection {
    int fd;
    const char* filename;
    bool filenameIsFd;
    int flags;
};

struct SimpleCommand {
    struct Redirection* redirections;
    size_t numRedirections;
    char** words;
    size_t numWords;
};

struct Pipeline {
    struct SimpleCommand* commands;
    size_t numCommands;
    bool bang;
};

struct CompleteCommand {
    struct Pipeline pipeline;
};

struct Parser {
    const struct Tokenizer* tokenizer;
    size_t offset;
};

enum ParserResult {
    PARSER_MATCH,
    PARSER_BACKTRACK,
    PARSER_SYNTAX,
    PARSER_ERROR,
    PARSER_NEWLINE,
};

void initParser(struct Parser* parser, const struct Tokenizer* tokenizer);
enum ParserResult parse(struct Parser* parser,
        struct CompleteCommand* command);
void freeCompleteCommand(struct CompleteCommand* command);

#endif
