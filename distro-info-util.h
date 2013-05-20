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
#define unused(x) x __attribute ((unused))
#else
#define likely(x)   (x)
#define unlikely(x) (x)
#define unused(x) x
#endif

#define ARRAY_SIZE(array) \
    (sizeof(array)/sizeof(array[0]))

#define MILESTONE_CREATED     "created"
#define MILESTONE_RELEASE     "release"
#define MILESTONE_EOL         "eol"
#define MILESTONE_EOL_SERVER  "eol-server"

#define MILESTONE(distro, name) \
    (distro)->milestones[milestone_to_index(name)]

#define DATA_DIR "/usr/share/distro-info"

#define OUTDATED_ERROR "Distribution data outdated.\n" \
    "Please check for an update for distro-info-data. " \
    "See /usr/share/doc/distro-info-data/README.Debian for details."

typedef struct {
    unsigned int year;
    unsigned int month;
    unsigned int day;
} date_t;

typedef struct {
    char *version;
    char *codename;
    char *series;
    date_t *milestones[4];
} distro_t;

typedef struct distro_elem_s {
    distro_t *distro;
    struct distro_elem_s *next;
} distro_elem_t;

static inline bool date_ge(const date_t *date1, const date_t *date2);
static inline bool created(const date_t *date, const distro_t *distro);
static inline bool released(const date_t *date, const distro_t *distro);
static inline bool eol(const date_t *date, const distro_t *distro);
static inline int milestone_to_index(const char *milestone);

#endif // __DISTRO_INFO_UTIL_H__
