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

/* sh/parser.c
 * Shell parser.
 */

#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "tokenizer.h"
#include "parser.h"

#define BACKTRACKING // specify that a function might return PARSER_BACKTRACK.

static BACKTRACKING enum ParserResult parseIoRedirect(struct Parser* parser,
        int fd, struct Redirection* result);
static enum ParserResult parsePipeline(struct Parser* parser,
        struct Pipeline* pipeline);
static enum ParserResult parseSimpleCommand(struct Parser* parser,
        struct SimpleCommand* command);
static void syntaxError(struct Token* token);

static void freePipeline(struct Pipeline* pipeline);
static void freeSimpleCommand(struct SimpleCommand* command);

static inline struct Token* getToken(struct Parser* parser) {
    if (parser->offset >= parser->tokenizer->numTokens) {
        return NULL;
    }
    return &parser->tokenizer->tokens[parser->offset];
}

void initParser(struct Parser* parser, const struct Tokenizer* tokenizer) {
    parser->tokenizer = tokenizer;
    parser->offset = 0;
}

enum ParserResult parse(struct Parser* parser,
        struct CompleteCommand* command) {
    enum ParserResult result = parsePipeline(parser, &command->pipeline);

    assert(result != PARSER_BACKTRACK);

    if (result == PARSER_MATCH &&
            parser->offset < parser->tokenizer->numTokens - 1) {
        syntaxError(getToken(parser));
        return PARSER_SYNTAX;
    }

    if (result == PARSER_SYNTAX) {
        syntaxError(getToken(parser));
    }
    return result;
}

static enum ParserResult parsePipeline(struct Parser* parser,
        struct Pipeline* pipeline) {
    pipeline->commands = NULL;
    pipeline->numCommands = 0;
    pipeline->bang = false;

    enum ParserResult result;

    struct Token* token = getToken(parser);
    if (!token) return PARSER_SYNTAX;

    while (token->type == TOKEN && strcmp(token->text, "!") == 0) {
        pipeline->bang = !pipeline->bang;
        parser->offset++;
        token = getToken(parser);
        if (!token) return PARSER_SYNTAX;
    }

    while (true) {
        struct SimpleCommand command;
        result = parseSimpleCommand(parser, &command);
        if (result != PARSER_MATCH) goto fail;

        if (!addToArray((void**) &pipeline->commands, &pipeline->numCommands,
                &command, sizeof(command))) {
            result = PARSER_ERROR;
            goto fail;
        }

        token = getToken(parser);
        if (!token) return PARSER_MATCH;
        if (token->type != OPERATOR || strcmp(token->text, "|") != 0) {
            return PARSER_MATCH;
        }

        parser->offset++;

        token = getToken(parser);
        if (!token) {
            result = PARSER_SYNTAX;
            goto fail;
        }

        while (token->type == OPERATOR && strcmp(token->text, "\n") == 0) {
            parser->offset++;
            token = getToken(parser);
            if (!token) {
                result = PARSER_NEWLINE;
                goto fail;
            }
        }
    }

    return PARSER_MATCH;

fail:
    freePipeline(pipeline);
    return result;
}

static enum ParserResult parseSimpleCommand(struct Parser* parser,
        struct SimpleCommand* command) {
    command->redirections = NULL;
    command->numRedirections = 0;
    command->words = NULL;
    command->numWords = 0;

    enum ParserResult result;

    struct Token* token = getToken(parser);
    assert(token);

    while (true) {
        if (token->type == IO_NUMBER || token->type == OPERATOR) {
            int fd = -1;
            if (token->type == IO_NUMBER) {
                fd = strtol(token->text, NULL, 10);
                parser->offset++;
            }

            struct Redirection redirection;
            result = parseIoRedirect(parser, fd, &redirection);

            if (result == PARSER_BACKTRACK) {
                if (command->numWords > 0 || command->numRedirections > 0) {
                    return PARSER_MATCH;
                }
                result = PARSER_SYNTAX;
                goto fail;
            }

            if (result != PARSER_MATCH) goto fail;

            if (!addToArray((void**) &command->redirections,
                    &command->numRedirections, &redirection,
                    sizeof(redirection))) {
                result = PARSER_ERROR;
                goto fail;
            }
        } else {
            assert(token->type == TOKEN);
            if (!addToArray((void**) &command->words, &command->numWords,
                    &token->text, sizeof(char*))) {
                result = PARSER_ERROR;
                goto fail;
            }
            parser->offset++;
        }

        token = getToken(parser);
        if (!token) return PARSER_MATCH;
    }

    return PARSER_MATCH;

fail:
    freeSimpleCommand(command);
    return result;
}

static BACKTRACKING enum ParserResult parseIoRedirect(struct Parser* parser,
        int fd, struct Redirection* result) {
    struct Token* token = getToken(parser);
    assert(token && token->type == OPERATOR);
    const char* operator = token->text;

    result->filenameIsFd = false;
    result->flags = 0;

    if (strcmp(operator, "<") == 0) {
        result->flags = O_RDONLY;
    } else if (strcmp(operator, ">") == 0) {
        result->flags = O_WRONLY | O_CREAT | O_TRUNC;
    } else if (strcmp(operator, ">|") == 0) {
        result->flags = O_WRONLY | O_CREAT | O_TRUNC;
    } else if (strcmp(operator, ">>") == 0) {
        result->flags = O_WRONLY | O_APPEND | O_CREAT;
    } else if (strcmp(operator, "<&") == 0) {
        result->filenameIsFd = true;
    } else if (strcmp(operator, ">&") == 0) {
        result->filenameIsFd = true;
    } else if (strcmp(operator, "<>") == 0) {
        result->flags = O_RDWR | O_CREAT;
    } else {
        return PARSER_BACKTRACK;
    }

    if (fd == -1) {
        fd = operator[0] == '<' ? 0 : 1;
    }
    result->fd = fd;

    parser->offset++;
    struct Token* filename = getToken(parser);
    if (!filename || filename->type != TOKEN) {
        return PARSER_SYNTAX;
    }
    result->filename = filename->text;

    parser->offset++;
    return PARSER_MATCH;
}

static void syntaxError(struct Token* token) {
    if (!token) {
        warnx("syntax error: unexpected end of file");
    } else if (strcmp(token->text, "\n") == 0) {
        warnx("syntax error: unexpected newline");
    } else {
        warnx("syntax error: unexpected '%s'", token->text);
    }
}

void freeCompleteCommand(struct CompleteCommand* command) {
    freePipeline(&command->pipeline);
}

static void freePipeline(struct Pipeline* pipeline) {
    for (size_t i = 0; i < pipeline->numCommands; i++) {
        freeSimpleCommand(&pipeline->commands[i]);
    }
    free(pipeline->commands);
}

static void freeSimpleCommand(struct SimpleCommand* command) {
    free(command->redirections);
    free(command->words);
}
