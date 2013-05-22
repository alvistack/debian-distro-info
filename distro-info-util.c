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
#include <assert.h>
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

/* All recognised dated database tags for milestones
 * (corresponding to distro_t date_t's).
 */
static char *milestones[] = {MILESTONE_CREATED
                            ,MILESTONE_RELEASE
                            ,MILESTONE_EOL
#ifdef UBUNTU
                            ,MILESTONE_EOL_SERVER
#endif
};

#define MILESTONE_COUNT ARRAY_SIZE(milestones)

static unsigned int days_in_month[] = {31, 28, 31, 30, 31, 30,
                                       31, 31, 30, 31, 30, 31};

static inline int milestone_to_index(const char *milestone) {
    int i;

    assert(milestone);

    for(i = 0; i < (int)MILESTONE_COUNT; i++) {
        if (!strcmp(milestone, milestones[i])) {
            return i;
        }
    }

    return -1;
}

static inline char *index_to_milestone(unsigned int i) {
    return milestones[i];
}

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
    return date->month >= 1 && date->month <= 12 &&
           date->day >= 1 &&
           date->day <= (is_leap_year(date->year) &&
           date->month == 1 ? 29 : days_in_month[date->month-1]);
}

static time_t date_to_secs(const date_t *date) {
    time_t seconds = 0;
    struct tm tm;

    assert(date);

    memset(&tm, '\0', sizeof(struct tm));

    tm.tm_year = date->year - 1900;
    tm.tm_mon = date->month - 1;
    tm.tm_mday = date->day;

    seconds = mktime(&tm);

    return seconds;
}

static inline size_t date_diff(const date_t *date1, const date_t *date2) {
    time_t time1;
    time_t time2;
    time_t diff;
    unsigned int days;

    time1 = date_to_secs(date1);
    time2 = date_to_secs(date2);

    diff = (time_t)difftime(time1, time2);

    days = (unsigned int)diff / (60 * 60 * 24);

    return days;
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
    return MILESTONE(distro, MILESTONE_CREATED) &&
           date_ge(date, MILESTONE(distro, MILESTONE_CREATED));
}

static inline bool released(const date_t *date, const distro_t *distro) {
    return *distro->version != '\0' &&
           MILESTONE(distro, MILESTONE_RELEASE) &&
           date_ge(date, MILESTONE(distro, MILESTONE_RELEASE));
}

static inline bool eol(const date_t *date, const distro_t *distro) {
    return MILESTONE(distro, MILESTONE_EOL) &&
           date_ge(date, MILESTONE(distro, MILESTONE_EOL))
#ifdef UBUNTU
           && (!MILESTONE(distro, MILESTONE_EOL_SERVER) ||
              (MILESTONE(distro, MILESTONE_EOL_SERVER) &&
               date_ge(date, MILESTONE(distro, MILESTONE_EOL_SERVER))))
#endif
    ;
}

// Filter callbacks

static bool filter_all(unused(const date_t *date),
                       unused(const distro_t *distro)) {
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
        if(distro_list && date_ge(MILESTONE(distro_list->distro,
                                            MILESTONE_CREATED),
                                  MILESTONE(selected, MILESTONE_CREATED))) {
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
        if(distro_list && date_ge(MILESTONE(distro_list->distro,
                                            MILESTONE_RELEASE),
                                  MILESTONE(selected, MILESTONE_RELEASE))) {
            selected = distro_list->distro;
        }
    }
    return selected;
}

static bool calculate_days(const distro_t *distro, const date_t *date,
                           int date_index, ssize_t *days) {
    const date_t *first;
    const date_t *second;
    date_t *milestone = NULL;
    int direction;

    if(date_index == milestone_to_index(MILESTONE_CREATED)) {
        milestone = MILESTONE(distro, MILESTONE_CREATED);
    } else if(date_index == milestone_to_index(MILESTONE_RELEASE)) {
        milestone = MILESTONE(distro, MILESTONE_RELEASE);
    } else if(date_index == milestone_to_index(MILESTONE_EOL)) {
        milestone = MILESTONE(distro, MILESTONE_EOL);
    }
#ifdef UBUNTU
    else if(date_index == milestone_to_index(MILESTONE_EOL_SERVER)) {
        milestone = MILESTONE(distro, MILESTONE_EOL_SERVER);
        if(!milestone)
            return false;
    }
#endif

    /* distro may not have specified a particular milestone date
     * (yet).
     */
    if (!milestone)
        return false;

    if(date_ge(date, milestone)) {
        first = date;
        second = milestone;
        direction = -1;
    } else {
        first = milestone;
        second = date;
        direction = 1;
    }

    *days = (ssize_t)date_diff(first, second) * direction;
    return true;
}

// Print callbacks

static bool print_codename(const distro_t *distro, const date_t *date,
                           int date_index) {
    ssize_t days;

    if(date_index == -1) {
        printf("%s\n", distro->series);
    } else {
        if(!calculate_days(distro, date, date_index, &days)) {
            printf("%s %s\n", distro->series, UNKNOWN_DAYS);
        } else {
            printf("%s %ld\n", distro->series, (long int)days);
        }
    }

    return true;
}

static bool print_fullname(const distro_t *distro, const date_t *date,
                           int date_index) {
    ssize_t days;

    if(date_index == -1) {
        printf(DISTRO_NAME " %s \"%s\"\n", distro->version, distro->codename);
    } else {
        if(!calculate_days(distro, date, date_index, &days)) {
            printf(DISTRO_NAME " %s \"%s\" %s\n", distro->version,
                    distro->codename, UNKNOWN_DAYS);
        } else {
            printf(DISTRO_NAME " %s \"%s\" %ld\n", distro->version,
                    distro->codename, (long int)days);
        }
    }
    return true;
}

static bool print_release(const distro_t *distro, const date_t *date,
                          int date_index) {
    ssize_t days;
    char *str;

    str = unlikely(*distro->version == '\0') ? distro->series : distro->version;

    if(date_index == -1) {
        printf("%s\n", str);
    } else {
        if(!calculate_days(distro, date, date_index, &days)) {
            printf("%s %s\n", distro->series, UNKNOWN_DAYS);
        } else {
            printf("%s %ld\n", str, (long int)days);
        }
    }

    return true;
}

// End of callbacks

static void free_data(distro_elem_t *list, char **content) {
    distro_elem_t *next;

    while(list != NULL) {
        next = list->next;
        free(MILESTONE(list->distro, MILESTONE_CREATED));
        free(MILESTONE(list->distro, MILESTONE_RELEASE));
        free(MILESTONE(list->distro, MILESTONE_EOL));
#ifdef UBUNTU
        free(MILESTONE(list->distro, MILESTONE_EOL_SERVER));
#endif
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
            int milestone_index;

            distro = malloc(sizeof(distro_t));
            distro->version = strsep(&line, ",");
            distro->codename = strsep(&line, ",");
            distro->series = strsep(&line, ",");

            for(milestone_index = 0; milestone_index < (int)MILESTONE_COUNT;
                milestone_index++) {
                distro->milestones[milestone_index] =
                    read_date(strsep(&line, ","), &failures, filename,
                              lineno, index_to_milestone(milestone_index));
            }

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

static bool filter_data(const distro_elem_t *distro_list, const date_t *date,
                        int date_index,
                        bool (*filter_cb)(const date_t*, const distro_t*),
                        bool (*print_cb)(const distro_t*, const date_t*, int)) {
    while(distro_list != NULL) {
        if(filter_cb(date, distro_list->distro)) {
            if(!print_cb(distro_list->distro, date, date_index)) {
                return false;
            }
        }
        distro_list = distro_list->next;
    }

    return false;
}

static const distro_t *get_distro(const distro_elem_t *distro_list,
                                  const date_t *date,
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
    int i;

    printf("Usage: " NAME " [options]\n"
           "\n"
           "Options:\n"
           "  -h  --help             show this help message and exit\n"
           "      --date=DATE        date for calculating the version (default: today)\n"
           "  -y[MILESTONE]          additionally, display days until milestone\n"
           "      --days=[MILESTONE] ("
          );

    for(i = 0; i < (int)MILESTONE_COUNT; i++)
        printf("%s%s", milestones[i],
                i+1 == MILESTONE_COUNT ? ")\n" : ", ");

    printf(""
#ifdef DEBIAN
           "      --alias=DIST       print the alias (stable, testing, unstable) relative to\n"
           "                         the distribution codename passed as an argument\n"
#endif
           "  -a  --all              list all known versions\n"
           "  -d  --devel            latest development version\n"
#ifdef UBUNTU
           "      --lts              latest long term support (LTS) version\n"
#endif
#ifdef DEBIAN
           "  -o  --oldstable        latest oldstable version\n"
#endif
           "  -s  --stable           latest stable version\n"
           "      --supported        list of all supported stable versions\n"
#ifdef DEBIAN
           "  -t  --testing          current testing version\n"
#endif
           "      --unsupported      list of all unsupported stable versions\n"
           "  -c  --codename         print the codename (default)\n"
           "  -f  --fullname         print the full name\n"
           "  -r  --release          print the release version\n"
           "\n"
           "See " NAME "(1) for more info.\n");
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
    bool show_days = false;
    char *content;
    date_t *date = NULL;
    distro_elem_t *distro_list;
    const distro_t *selected;
    int i;
    int date_index = -1;
    int option;
    int option_index;
    int return_value = EXIT_SUCCESS;
    int selected_filters = 0;
    bool (*filter_cb)(const date_t*, const distro_t*) = NULL;
    const distro_t *(*select_cb)(const distro_elem_t*) = NULL;
    bool (*print_cb)(const distro_t*, const date_t*, int) = print_codename;
#ifdef DEBIAN
    char *alias_codename = NULL;
#endif

    const struct option long_options[] = {
        {"help",        no_argument,       NULL, 'h' },
        {"date",        required_argument, NULL, 'D' },
        {"all",         no_argument,       NULL, 'a' },
        {"days",        optional_argument, NULL, 'y' },
        {"devel",       no_argument,       NULL, 'd' },
        {"stable",      no_argument,       NULL, 's' },
        {"supported",   no_argument,       NULL, 'S' },
        {"unsupported", no_argument,       NULL, 'U' },
        {"codename",    no_argument,       NULL, 'c' },
        {"fullname",    no_argument,       NULL, 'f' },
        {"release",     no_argument,       NULL, 'r' },
#ifdef DEBIAN
        {"alias",       required_argument, NULL, 'A' },
        {"oldstable",   no_argument,       NULL, 'o' },
        {"testing",     no_argument,       NULL, 't' },
#endif
#ifdef UBUNTU
        {"lts",         no_argument,       NULL, 'L' },
#endif
        {NULL,          0,                 NULL, '\0' }
    };

#ifdef UBUNTU
    const char *short_options = "hadscrfy::";
#endif
#ifdef DEBIAN
    const char *short_options = "hadscrfoty::";
#endif

    // Suppress error messages from getopt_long
    opterr = 0;

    while ((option = getopt_long(argc, argv, short_options,
                                 long_options, &option_index)) != -1) {
        switch (option) {
#ifdef DEBIAN
            case 'A':
                // Only long option --alias is used
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
                selected_filters++;
                alias_codename = optarg;
                break;
#endif

            case 'a':
                selected_filters++;
                filter_cb = filter_all;
                select_cb = NULL;
                break;

            case 'c':
                print_cb = print_codename;
                break;

            case 'd':
                selected_filters++;
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
                selected_filters++;
                filter_cb = filter_lts;
                select_cb = select_latest_release;
                break;
#endif

#ifdef DEBIAN
            case 'o':
                selected_filters++;
                filter_cb = filter_oldstable;
                select_cb = select_oldstable;
                break;
#endif

            case 'r':
                print_cb = print_release;
                break;

            case 's':
                selected_filters++;
                filter_cb = filter_stable;
                select_cb = select_latest_release;
                break;

            case 'S':
                // Only long option --supported is used
                selected_filters++;
                filter_cb = filter_supported;
                select_cb = NULL;
                break;

#ifdef DEBIAN
            case 't':
                selected_filters++;
                filter_cb = filter_testing;
                select_cb = select_latest_created;
                break;
#endif

            case 'U':
                // Only long option --unsupported is used
                selected_filters++;
                filter_cb = filter_unsupported;
                select_cb = NULL;
                break;

            case 'y':
                show_days = true;
                if(optarg) {
                    date_index = milestone_to_index(optarg);
                    if(date_index < 0) {
                        fprintf(stderr, NAME ": invalid milestone: %s\n",
                                optarg);
                        return EXIT_FAILURE;
                    }
                }
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

    if(show_days && date_index < 0) {
        date_index = milestone_to_index(MILESTONE_RELEASE);
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

    if(unlikely(selected_filters != 1)) {
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
            fprintf(stderr, NAME ": " OUTDATED_ERROR "\n");
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
        filter_data(distro_list, date, date_index, filter_cb, print_cb);
    } else {
        selected = get_distro(distro_list, date, filter_cb, select_cb);
        if(selected == NULL) {
            fprintf(stderr, NAME ": " OUTDATED_ERROR "\n");
            return_value = EXIT_FAILURE;
        } else {
            if(!print_cb(selected, date, date_index)) {
                return_value = EXIT_FAILURE;
            }
        }
    }
    free(date);
    free_data(distro_list, &content);
    return return_value;
}
