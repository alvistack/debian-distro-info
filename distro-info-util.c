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

// C standard libraries
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "distro-info-util.h"

static char *read_full_file(const char *filename) {
    char *content;
    FILE *f;
    size_t size;
    struct stat stat;

    f = fopen(filename, "r");
    if(unlikely(f == NULL)) {
        fprintf(stderr, NAME ": Failed to open %s: %s\n", filename,
                strerror(errno));
        return NULL;
    }
    fstat(fileno(f), &stat);
    if(unlikely(stat.st_size < 0)) {
        fprintf(stderr, NAME ": %s has a negative file size.\n", filename);
        return NULL;
    }
    size = stat.st_size;
    content = malloc(size + 1);
    if(unlikely(fread(content, sizeof(char), size, f) != size)) {
        fprintf(stderr, NAME ": Failed to read %zu bytes from %s.\n", size,
                filename);
        free(content);
        return NULL;
    }
    content[size] = '\0';
    fclose(f);
    return content;
}

static inline bool is_leap_year(const int year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

static inline bool is_valid_date(const date_t *date) {
    unsigned int days_in_month[] = {31, 28, 31, 30, 31, 30,
                                    31, 31, 30, 31, 30, 31};
    if(is_leap_year(date->year)) {
        days_in_month[1] = 29;
    }
    return date->month >= 1 && date->month <= 12 &&
           date->day >= 1 && date->day <= days_in_month[date->month-1];
}

static inline bool is_valid_codename(const char *codename) {
    // Only codenames with lowercase ASCII letters are accepted
    return strlen(codename) > 0 &&
           strspn(codename, "abcdefghijklmnopqrstuvwxyz") == strlen(codename);
}

// Read an ISO 8601 formatted date
static date_t *read_date(const char *s, int *failures, const char *filename,
                         const int lineno, const char *column) {
    date_t *date = NULL;
    int n;

    if(s) {
        date = malloc(sizeof(date_t));
        n = sscanf(s, "%u-%u-%u", &date->year, &date->month, &date->day);
        if(unlikely(n != 3 || !is_valid_date(date))) {
            fprintf(stderr, NAME ": Invalid date `%s' in file `%s' at line %i "
                    "in column `%s'.\n", s, filename, lineno, column);
            (*failures)++;
            free(date);
            date = NULL;
        }
    }
    return date;
}

static inline bool date_ge(const date_t *date1, const date_t *date2) {
    return date1->year > date2->year ||
           (date1->year == date2->year && date1->month > date2->month) ||
           (date1->year == date2->year && date1->month == date2->month &&
            date1->day >= date2->day);
}

static inline bool created(const date_t *date, const distro_t *distro) {
    return distro->created && date_ge(date, distro->created);
}

static inline bool released(const date_t *date, const distro_t *distro) {
    return *distro->version != '\0' &&
           distro->release && date_ge(date, distro->release);
}

static inline bool eol(const date_t *date, const distro_t *distro) {
    return distro->eol && date_ge(date, distro->eol) && (!distro->eol_server ||
           (distro->eol_server && date_ge(date, distro->eol_server)));
}

// Filter callbacks

static bool filter_all(const date_t *date, const distro_t *distro) {
    (void)date;
    (void)distro;
    return true;
}

static bool filter_stable(const date_t *date, const distro_t *distro) {
    return released(date, distro) && !eol(date, distro);
}

static bool filter_supported(const date_t *date, const distro_t *distro) {
    return created(date, distro) && !eol(date, distro);
}

static bool filter_unsupported(const date_t *date, const distro_t *distro) {
    return created(date, distro) && eol(date, distro);
}

// Select callbacks

static const distro_t *select_latest_created(const distro_elem_t *distro_list) {
    const distro_t *selected;

    selected = distro_list->distro;
    while(distro_list != NULL) {
        distro_list = distro_list->next;
        if(distro_list && date_ge(distro_list->distro->created,
                                  selected->created)) {
            selected = distro_list->distro;
        }
    }
    return selected;
}

static const distro_t *select_latest_release(const distro_elem_t *distro_list) {
    const distro_t *selected;

    selected = distro_list->distro;
    while(distro_list != NULL) {
        distro_list = distro_list->next;
        if(distro_list && date_ge(distro_list->distro->release,
                                  selected->release)) {
            selected = distro_list->distro;
        }
    }
    return selected;
}

// Print callbacks

static void print_codename(const distro_t *distro) {
    printf("%s\n", distro->series);
}

static void print_fullname(const distro_t *distro) {
    printf(DISTRO_NAME " %s \"%s\"\n", distro->version, distro->codename);
}

static void print_release(const distro_t *distro) {
    if(unlikely(*distro->version == '\0')) {
        printf("%s\n", distro->series);
    } else {
        printf("%s\n", distro->version);
    }
}

// End of callbacks

static void free_data(distro_elem_t *list, char **content) {
    distro_elem_t *next;

    while(list != NULL) {
        next = list->next;
        free(list->distro->created);
        free(list->distro->release);
        free(list->distro->eol);
        free(list->distro->eol_server);
        free(list->distro);
        free(list);
        list = next;
    }

    free(*content);
    *content = NULL;
}

static distro_elem_t *read_data(const char *filename, char **content) {
    char *data;
    char *line;
    distro_elem_t *current;
    distro_elem_t *distro_list = NULL;
    distro_elem_t *last = NULL;
    distro_t *distro;
    int lineno;
    int failures = 0;

    data = *content = read_full_file(filename);
    line = strsep(&data, "\n");
    lineno = 1;
    if(unlikely(strcmp(CSV_HEADER, line) != 0)) {
        fprintf(stderr, NAME ": Header `%s' in file `%s' does not match "
                "excatly `" CSV_HEADER "'.\n", line, filename);
        failures++;
    }

    while((line = strsep(&data, "\n")) != NULL) {
        lineno++;
        // Ignore empty lines and comments (starting with #).
        if(likely(*line != '\0' && *line != '#')) {
            distro = malloc(sizeof(distro_t));
            distro->version = strsep(&line, ",");
            distro->codename = strsep(&line, ",");
            distro->series = strsep(&line, ",");
            distro->created = read_date(strsep(&line, ","), &failures, filename,
                                        lineno, "created");
            distro->release = read_date(strsep(&line, ","), &failures, filename,
                                        lineno, "release");
            distro->eol = read_date(strsep(&line, ","), &failures, filename,
                                    lineno, "eol");
            distro->eol_server = read_date(strsep(&line, ","), &failures,
                                           filename, lineno, "eol-server");

            current = malloc(sizeof(distro_elem_t));
            current->distro = distro;
            current->next = NULL;
            if(last == NULL) {
                distro_list = current;
            } else {
                last->next = current;
            }
            last = current;
        }
    }

    if(unlikely(distro_list == NULL)) {
        fprintf(stderr, NAME ": No data found in file `%s'.\n", filename);
        failures++;
    }

    if(unlikely(failures > 0)) {
        free_data(distro_list, content);
        distro_list = NULL;
    }

    return distro_list;
}

static void filter_data(const distro_elem_t *distro_list, const date_t *date,
                       bool (*filter_cb)(const date_t*, const distro_t*),
                       void (*print_cb)(const distro_t*)) {
    while(distro_list != NULL) {
        if(filter_cb(date, distro_list->distro)) {
            print_cb(distro_list->distro);
        }
        distro_list = distro_list->next;
    }
}

static const distro_t *get_distro(const distro_elem_t *distro_list, const date_t *date,
                                  bool (*filter_cb)(const date_t*, const distro_t*),
                                  const distro_t *(*select_cb)(const distro_elem_t*)) {
    distro_elem_t *current;
    distro_elem_t *filtered_list = NULL;
    distro_elem_t *last = NULL;
    const distro_t *selected;

    while(distro_list != NULL) {
        if(filter_cb(date, distro_list->distro)) {
            current = malloc(sizeof(distro_elem_t));
            current->distro = distro_list->distro;
            current->next = NULL;
            if(last == NULL) {
                filtered_list = current;
            } else {
                last->next = current;
            }
            last = current;
        }
        distro_list = distro_list->next;
    }

    if(filtered_list == NULL) {
        selected = NULL;
    } else {
        selected = select_cb(filtered_list);
    }

    while(filtered_list != NULL) {
        current = filtered_list->next;
        free(filtered_list);
        filtered_list = current;
    }

    return selected;
}

static void print_help(void) {
    printf("Usage: " NAME " [options]\n\
\n\
Options:\n\
  -h  --help         show this help message and exit\n\
      --date=DATE    date for calculating the version (default: today)\n"
#ifdef DEBIAN
"      --alias=DIST   print the alias (stable, testing, unstable) relative to\n\
                     the distribution codename passed as an argument\n"
#endif
"  -a  --all          list all known versions\n\
  -d  --devel        latest development version\n"
#ifdef UBUNTU
"      --lts          latest long term support (LTS) version\n"
#endif
#ifdef DEBIAN
"  -o  --oldstable    latest oldstable version\n"
#endif
"  -s  --stable       latest stable version\n\
      --supported    list of all supported stable versions\n"
#ifdef DEBIAN
"  -t  --testing      current testing version\n"
#endif
"      --unsupported  list of all unsupported stable versions\n\
  -c  --codename     print the codename (default)\n\
  -r  --release      print the release version\n\
  -f  --fullname     print the full name\n\
\n\
See " NAME "(1) for more info.\n");
}

static inline int not_exactly_one(void) {
    fprintf(stderr, NAME ": You have to select exactly one of "
#ifdef DEBIAN
            "--alias, "
#endif
            "--all, --devel, "
#ifdef UBUNTU
            "--lts, "
#endif
#ifdef DEBIAN
            "--oldstable, "
#endif
            "--stable, --supported, "
#ifdef DEBIAN
            "--testing, "
#endif
            "--unsupported.\n");
    return EXIT_FAILURE;
}

int main(int argc, char *argv[]) {
    char *content;
    date_t *date = NULL;
    distro_elem_t *distro_list;
    const distro_t *selected;
    int i;
    int option;
    int option_index;
    int return_value = EXIT_SUCCESS;
    bool (*filter_cb)(const date_t*, const distro_t*) = NULL;
    const distro_t *(*select_cb)(const distro_elem_t*) = NULL;
    void (*print_cb)(const distro_t*) = print_codename;
#ifdef DEBIAN
    char *alias_codename = NULL;
#endif

    const struct option long_options[] = {
        {"help",        no_argument,       NULL, 'h' },
        {"date",        required_argument, NULL, 'D' },
#ifdef DEBIAN
        {"alias",       required_argument, NULL, 'A' },
#endif
        {"all",         no_argument,       NULL, 'a' },
        {"devel",       no_argument,       NULL, 'd' },
        {"stable",      no_argument,       NULL, 's' },
        {"supported",   no_argument,       NULL, 'S' },
        {"unsupported", no_argument,       NULL, 'U' },
        {"codename",    no_argument,       NULL, 'c' },
        {"release",     no_argument,       NULL, 'r' },
        {"fullname",    no_argument,       NULL, 'f' },
#ifdef UBUNTU
        {"lts",         no_argument,       NULL, 'L' },
#endif
#ifdef DEBIAN
        {"oldstable",   no_argument,       NULL, 'o' },
        {"testing",     no_argument,       NULL, 't' },
#endif
        {NULL,          0,                 NULL, '\0' }
    };

#ifdef UBUNTU
    const char *short_options = "hadscrf";
#endif
#ifdef DEBIAN
    const char *short_options = "hadscrfot";
#endif

    // Suppress error messages from getopt_long
    opterr = 0;

    while ((option = getopt_long(argc, argv, short_options,
                                 long_options, &option_index)) != -1) {
        switch (option) {
#ifdef DEBIAN
            case 'A':
                // Only long option --alias is used
                if(unlikely(filter_cb != NULL)) {
                    free(date);
                    return not_exactly_one();
                }
                if(unlikely(alias_codename != NULL)) {
                    fprintf(stderr, NAME ": --alias requested multiple times.\n");
                    free(date);
                    return EXIT_FAILURE;
                }
                if(!is_valid_codename(optarg)) {
                    fprintf(stderr, NAME ": invalid distribution codename `%s'\n",
                            optarg);
                    free(date);
                    return EXIT_FAILURE;
                }
                alias_codename = optarg;
                break;
#endif

            case 'a':
                if(unlikely(filter_cb != NULL)) {
                    free(date);
                    return not_exactly_one();
                }
                filter_cb = filter_all;
                select_cb = NULL;
                break;

            case 'c':
                print_cb = print_codename;
                break;

            case 'd':
                if(unlikely(filter_cb != NULL)) {
                    free(date);
                    return not_exactly_one();
                }
                filter_cb = filter_devel;
#ifdef UBUNTU
                select_cb = select_latest_created;
#endif
#ifdef DEBIAN
                select_cb = select_first;
#endif
                break;

            case 'D':
                // Only long option --date is used
                if(unlikely(date != NULL)) {
                    fprintf(stderr, NAME ": Date specified multiple times.\n");
                    free(date);
                    return EXIT_FAILURE;
                }
                date = malloc(sizeof(date_t));
                i = sscanf(optarg, "%u-%u-%u", &date->year, &date->month,
                           &date->day);
                if(i != 3 || !is_valid_date(date)) {
                    fprintf(stderr, NAME ": invalid date `%s'\n", optarg);
                    free(date);
                    return EXIT_FAILURE;
                }
                break;

            case 'f':
                print_cb = print_fullname;
                break;

            case 'h':
                print_help();
                free(date);
                return EXIT_SUCCESS;

#ifdef UBUNTU
            case 'L':
                // Only long option --lts is used
                if(unlikely(filter_cb != NULL)) {
                    free(date);
                    return not_exactly_one();
                }
                filter_cb = filter_lts;
                select_cb = select_latest_release;
                break;
#endif

#ifdef DEBIAN
            case 'o':
                if(unlikely(filter_cb != NULL)) {
                    free(date);
                    return not_exactly_one();
                }
                filter_cb = filter_oldstable;
                select_cb = select_oldstable;
                break;
#endif

            case 'r':
                print_cb = print_release;
                break;

            case 's':
                if(unlikely(filter_cb != NULL)) {
                    free(date);
                    return not_exactly_one();
                }
                filter_cb = filter_stable;
                select_cb = select_latest_release;
                break;

            case 'S':
                // Only long option --supported is used
                if(unlikely(filter_cb != NULL)) {
                    free(date);
                    return not_exactly_one();
                }
                filter_cb = filter_supported;
                select_cb = NULL;
                break;

#ifdef DEBIAN
            case 't':
                if(unlikely(filter_cb != NULL)) {
                    free(date);
                    return not_exactly_one();
                }
                filter_cb = filter_testing;
                select_cb = select_latest_created;
                break;
#endif

            case 'U':
                // Only long option --unsupported is used
                if(unlikely(filter_cb != NULL)) {
                    free(date);
                    return not_exactly_one();
                }
                filter_cb = filter_unsupported;
                select_cb = NULL;
                break;

            case '?':
                if(optopt == '\0') {
                    // Long option failed
                    fprintf(stderr, NAME ": unrecognized option `%s'\n",
                            argv[optind-1]);
#ifdef DEBIAN
                } else if(optopt == 'A') {
                    fprintf(stderr, NAME ": option `--alias' requires "
                            "an argument DIST\n");
#endif
                } else if(optopt == 'D') {
                    fprintf(stderr, NAME ": option `--date' requires "
                            "an argument DATE\n");
                } else {
                    fprintf(stderr, NAME ": unrecognized option `-%c'\n",
                            optopt);
                }
                free(date);
                return EXIT_FAILURE;
                break;

            default:
                fprintf(stderr, NAME ": getopt returned character code %i. "
                        "Please file a bug report.\n", option);
                free(date);
                return EXIT_FAILURE;
        }
    }

    if(unlikely(optind < argc)) {
        fprintf(stderr, NAME ": unrecognized arguments: %s", argv[optind]);
        for(i = optind + 1; i < argc; i++) {
            fprintf(stderr, " %s", argv[i]);
        }
        fprintf(stderr, "\n");
        free(date);
        return EXIT_FAILURE;
    }

#ifdef DEBIAN
    if(unlikely(filter_cb == NULL && !alias_codename) ||
       unlikely(filter_cb != NULL && alias_codename)) {
#else
    if(unlikely(filter_cb == NULL)) {
#endif
        free(date);
        return not_exactly_one();
    }

    if(unlikely(date == NULL)) {
        time_t time_now = time(NULL);
        struct tm *now = gmtime(&time_now);
        date = malloc(sizeof(date_t));
        date->year = 1900 + now->tm_year;
        date->month = 1 + now->tm_mon;
        date->day = now->tm_mday;
    }

    distro_list = read_data(DATA_DIR "/" CSV_NAME ".csv", &content);
    if(unlikely(distro_list == NULL)) {
        free(date);
        return EXIT_FAILURE;
    }

#ifdef DEBIAN
    if(alias_codename) {
        const distro_t *stable = get_distro(distro_list, date, filter_stable,
                                            select_latest_release);
        const distro_t *testing = get_distro(distro_list, date, filter_testing,
                                             select_latest_created);
        const distro_t *unstable = get_distro(distro_list, date, filter_devel,
                                              select_first);
        if(unlikely(stable == NULL || testing == NULL || unstable == NULL)) {
            fprintf(stderr, NAME ": Distribution data outdated.\n");
            return_value = EXIT_FAILURE;
        } else if(strcmp(stable->series, alias_codename) == 0) {
            printf("stable\n");
        } else if(strcmp(testing->series, alias_codename) == 0) {
            printf("testing\n");
        } else if(strcmp(unstable->series, alias_codename) == 0) {
            printf("unstable\n");
        } else {
            printf("%s\n", alias_codename);
        }
        free(date);
        free_data(distro_list, &content);
        return return_value;
    }
#endif

    if(select_cb == NULL) {
        filter_data(distro_list, date, filter_cb, print_cb);
    } else {
        selected = get_distro(distro_list, date, filter_cb, select_cb);
        if(selected == NULL) {
            fprintf(stderr, NAME ": Distribution data outdated.\n");
            return_value = EXIT_FAILURE;
        } else {
            print_cb(selected);
        }
    }
    free(date);
    free_data(distro_list, &content);
    return return_value;
}
