/* Copyright (c) 2016, Dennis Wölfing
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

/* libc/src/stdlib/atexit.c
 * Registers a function that should run when the process terminates.
 */

#include <stdlib.h>
#include <sys/types.h>

#define ATEXIT_MAX 32
static void (*atexitHandlers[ATEXIT_MAX])(void);

int atexit(void (*func)(void)) {
    for (size_t i = 0; i < ATEXIT_MAX; i++) {
        if (!atexitHandlers[i]) {
            atexitHandlers[i] = func;
            return 0;
        }
    }
    return -1;
}

__attribute__((destructor))
void __callAtexitHandlers(void) {
    for (ssize_t i = ATEXIT_MAX - 1; i >= 0; i--) {
        if (atexitHandlers[i]) {
            atexitHandlers[i]();
        }
    }
}
