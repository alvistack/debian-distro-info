/*
 * Copyright (C) 2012, Benjamin Drung <bdrung@debian.org>
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

#ifndef __DISTRO_INFO_UTIL_H__
#define __DISTRO_INFO_UTIL_H__

// C standard libraries
#include <stdbool.h>

#ifdef __GNUC__
#define likely(x)   __builtin_expect((x),1)
#define unlikely(x) __builtin_expect((x),0)
#else
#define likely(x)   (x)
#define unlikely(x) (x)
#endif

#define DATA_DIR "/usr/share/distro-info"

typedef struct {
    unsigned int year;
    unsigned int month;
    unsigned int day;
} date_t;

typedef struct {
    char *version;
    char *codename;
    char *series;
    date_t *created;
    date_t *release;
    date_t *eol;
    date_t *eol_server;
} distro_t;

typedef struct distro_elem_s {
    distro_t *distro;
    struct distro_elem_s *next;
} distro_elem_t;

static inline bool date_ge(const date_t *date1, const date_t *date2);
static inline bool created(const date_t *date, const distro_t *distro);
static inline bool released(const date_t *date, const distro_t *distro);
static inline bool eol(const date_t *date, const distro_t *distro);

#endif // __DISTRO_INFO_UTIL_H__
