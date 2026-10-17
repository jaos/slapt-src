/*
 * Copyright (C) 2010-2026 Jason Woodward <woodwardj at jaos dot org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#define _GNU_SOURCE
#include <ctype.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "source.h"
#include "config.h"

extern struct utsname uname_v;

static char *filename_from_url(const char *url);
static char *add_part_to_url(const char *url, const char *part);
static bool sb_chars_allowed(const char *name);
static bool sb_name_valid(const char *name);
static bool sb_path_valid(const char *path);

/* execute a command via fork/exec, bypassing the shell */
static int slapt_src_exec(const char *prog, char *const argv[])
{
    pid_t pid = fork();

    if (pid < 0) {
        fprintf(stderr, "fork failed: %s\n", strerror(errno));
        return -1;
    }

    if (pid == 0) {
        execvp(prog, argv);
        fprintf(stderr, "Failed to exec %s: %s\n", prog, strerror(errno));
        _exit(EXIT_FAILURE);
    }

    int status = 0;
    int wait_r = 0;
    do {
        wait_r = waitpid(pid, &status, 0);
    } while (wait_r == -1 && errno == EINTR);

    if (wait_r == -1) {
        fprintf(stderr, "waitpid failed: %s\n", strerror(errno));
        return -1;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        fprintf(stderr, "killed by signal %d\n", WTERMSIG(status));
        return 128 + WTERMSIG(status);
    }
    return -1;
}

slapt_src_config *slapt_src_config_init(void)
{
    slapt_src_config *config = slapt_malloc(sizeof *config);
    config->sources = slapt_vector_t_init(free);
    config->builddir = NULL;
    config->pkgext = NULL;
    config->pkgtag = NULL;
    config->postcmd = NULL;
    config->do_dep = false;
    config->prompt = true;
    return config;
}

void slapt_src_config_free(slapt_src_config *config)
{
    slapt_vector_t_free(config->sources);
    if (config->builddir != NULL)
        free(config->builddir);
    if (config->pkgext != NULL)
        free(config->pkgext);
    if (config->pkgtag != NULL)
        free(config->pkgtag);
    if (config->postcmd != NULL)
        free(config->postcmd);
    free(config);
}

static const char *skip_spaces(const char *s)
{
    while (*s != '\0' && isspace((unsigned char)*s))
        s++;
    return s;
}

static void strip_rspace(char *s)
{
    size_t l = strlen(s);
    while (l > 0 && isspace((unsigned char)s[l - 1])) {
        s[--l] = '\0';
    }
}

slapt_src_config *slapt_src_read_config(const char *filename)
{
    FILE *rc = slapt_open_file(filename, "r");
    if (rc == NULL) {
        fprintf(stderr, gettext("Failed to read %s\n"), filename);
        exit(EXIT_FAILURE);
    }

    slapt_src_config *config = slapt_src_config_init();

    char *buffer = NULL;
    size_t gb_length = 0;
    ssize_t g_size;
    while ((g_size = getline(&buffer, &gb_length, rc)) != EOF) {
        if ((strchr(buffer, '#') != NULL))
            continue;

        buffer[strcspn(buffer, "\r\n")] = '\0';
        const char *nospace_buffer = skip_spaces(buffer);

        if (strncmp(nospace_buffer, SLAPT_SRC_SOURCE_TOKEN, strlen(SLAPT_SRC_SOURCE_TOKEN)) == 0) {
            const char *value = skip_spaces(nospace_buffer + strlen(SLAPT_SRC_SOURCE_TOKEN));
            if (*value != '\0') {
                char *source = strdup(value);
                if (source == NULL)
                    exit(EXIT_FAILURE);
                strip_rspace(source);
                if (source[strlen(source) - 1] != '/') {
                    char *fixed = add_part_to_url(source, "/");
                    free(source);
                    source = fixed;
                }
                slapt_vector_t_add(config->sources, strdup(source));
                free(source);
            }
        } else if (strncmp(nospace_buffer, SLAPT_SRC_BUILDDIR_TOKEN, strlen(SLAPT_SRC_BUILDDIR_TOKEN)) == 0) {
            const char *value = skip_spaces(nospace_buffer + strlen(SLAPT_SRC_BUILDDIR_TOKEN));
            if (*value != '\0') {
                free(config->builddir);
                config->builddir = strdup(value);
                strip_rspace(config->builddir);
            }

        } else if (strncmp(nospace_buffer, SLAPT_SRC_PKGEXT_TOKEN, strlen(SLAPT_SRC_PKGEXT_TOKEN)) == 0) {
            const char *value = skip_spaces(nospace_buffer + strlen(SLAPT_SRC_PKGEXT_TOKEN));
            if (*value != '\0') {
                free(config->pkgext);
                config->pkgext = strdup(value);
                strip_rspace(config->pkgext);
            }

        } else if (strncmp(nospace_buffer, SLAPT_SRC_PKGTAG_TOKEN, strlen(SLAPT_SRC_PKGTAG_TOKEN)) == 0) {
            const char *value = skip_spaces(nospace_buffer + strlen(SLAPT_SRC_PKGTAG_TOKEN));
            if (*value != '\0') {
                free(config->pkgtag);
                config->pkgtag = strdup(value);
                strip_rspace(config->pkgtag);
            }
        }
    }

    /* sanity checks */
    if (config->builddir == NULL) {
        fprintf(stderr, gettext("No BUILDDIR specified in the configuration\n"));
        exit(EXIT_FAILURE);
    }

    fclose(rc);
    if (buffer != NULL)
        free(buffer);

    return config;
}

slapt_src_slackbuild *slapt_src_slackbuild_init(void)
{
    slapt_src_slackbuild *sb = slapt_malloc(sizeof *sb);
    sb->name = NULL;
    sb->version = NULL;
    sb->location = NULL;
    sb->sb_source_url = NULL;
    sb->short_desc = NULL;
    sb->download = NULL;
    sb->download_x86_64 = NULL;
    sb->md5sum = NULL;
    sb->md5sum_x86_64 = NULL;
    sb->requires = NULL;
    sb->files = slapt_vector_t_init(free);

    return sb;
}

void slapt_src_slackbuild_free(slapt_src_slackbuild *sb)
{
    if (sb->name != NULL)
        free(sb->name);
    if (sb->version != NULL)
        free(sb->version);
    if (sb->location != NULL)
        free(sb->location);
    if (sb->sb_source_url != NULL)
        free(sb->sb_source_url);
    if (sb->download != NULL)
        free(sb->download);
    if (sb->download_x86_64 != NULL)
        free(sb->download_x86_64);
    if (sb->md5sum != NULL)
        free(sb->md5sum);
    if (sb->md5sum_x86_64 != NULL)
        free(sb->md5sum_x86_64);
    if (sb->short_desc != NULL)
        free(sb->short_desc);
    if (sb->requires != NULL)
        free(sb->requires);

    slapt_vector_t_free(sb->files);

    free(sb);
}

bool slapt_src_update_slackbuild_cache(const slapt_src_config *config)
{
    bool rval = true;
    slapt_config_t *slapt_config = slapt_config_t_init();
    if (strlen(config->builddir) >= sizeof slapt_config->working_dir) {
        fprintf(stderr, gettext("BUILDDIR too long\n"));
        exit(EXIT_FAILURE);
    }
    slapt_strlcpy(slapt_config->working_dir, config->builddir, sizeof slapt_config->working_dir);

    slapt_vector_t *slackbuilds = slapt_vector_t_init((slapt_vector_t_free_function)slapt_src_slackbuild_free);

    slapt_vector_t_foreach(const char *, url, config->sources) {
        slapt_vector_t *sbs = NULL;
        const char *files[] = {SLAPT_SRC_SOURCES_LIST_GZ, SLAPT_SRC_SOURCES_LIST, NULL};

        printf(gettext("Fetching slackbuild list from %s..."), url);

        for (int fc = 0; files[fc] != NULL; fc++) {
            char *filename = slapt_gen_filename_from_url(slapt_config, url, files[fc]);
            char *local_head = slapt_read_head_cache(filename);
            char *head = slapt_head_mirror_data(url, files[fc]);

            /* is it cached ? */
            if (head != NULL && local_head != NULL && strcmp(head, local_head) == 0) {
                printf(gettext("Cached\n"));
                sbs = slapt_src_get_slackbuilds_from_file(filename);
            } else {
                FILE *f = slapt_open_file(filename, "w+b");
                if (f == NULL) {
                    exit(EXIT_FAILURE);
                }

                const char *err = slapt_get_mirror_data_from_source(f, slapt_config, url, files[fc]);
                fclose(f);

                if (!err) {
                    printf(gettext("Done\n"));
                    sbs = slapt_src_get_slackbuilds_from_file(filename);

                    if (head != NULL)
                        slapt_write_head_cache(head, filename);
                } else {
                    if (fc != 0) { // non GZ attempt
                        fprintf(stderr, gettext("Download failed: %s\n"), err);
                    }
                    slapt_clear_head_cache(filename);
                }
            }

            free(filename);
            free(local_head);
            free(head);

            if (sbs != NULL)
                break;
        }

        if (sbs != NULL) {
            slapt_vector_t_foreach(slapt_src_slackbuild *, sb, sbs) {
                if (sb->sb_source_url == NULL)
                    sb->sb_source_url = strdup(url);
                slapt_vector_t_add(slackbuilds, sb);
            }
            sbs->free_function = NULL; /* don't free the slackbuilds here */
            slapt_vector_t_free(sbs);
        } else {
            rval = false;
        }
    }

    char *datafile = slapt_gen_abs_path(config->builddir, SLAPT_SRC_DATA_FILE);

    if (slackbuilds->size == 0 || rval == false) {
        struct stat data_stat;
        if (stat(datafile, &data_stat) == 0 && data_stat.st_size > 0) {
            fprintf(stderr, gettext("Refusing to overwrite cache with incomplete list\n"));
            exit(EXIT_FAILURE);
        }
    }

    slapt_src_write_slackbuilds_to_file(slackbuilds, datafile);
    free(datafile);

    slapt_config_t_free(slapt_config);
    slapt_vector_t_free(slackbuilds);
    return rval;
}

static int sb_cmp(const void *a, const void *b)
{
    slapt_src_slackbuild *sb1 = *(slapt_src_slackbuild *const *)a;
    slapt_src_slackbuild *sb2 = *(slapt_src_slackbuild *const *)b;

    const int cmp = strcmp(sb1->name, sb2->name);
    if (cmp != 0)
        return cmp;
    else
        return slapt_pkg_t_cmp_versions(sb1->version, sb2->version);
}

void slapt_src_write_slackbuilds_to_file(slapt_vector_t *sbs, const char *datafile)
{
    FILE *f = slapt_open_file(datafile, "w+b");
    if (f == NULL) {
        fprintf(stderr, gettext("Failed to open %s for writing\n"), datafile);
        exit(EXIT_FAILURE);
    }

    slapt_vector_t_sort(sbs, sb_cmp);

    slapt_vector_t_foreach(const slapt_src_slackbuild *, sb, sbs) {
        /* write out package data */
        fprintf(f, "SLACKBUILD NAME: %s\n", sb->name);
        fprintf(f, "SLACKBUILD SOURCEURL: %s\n", sb->sb_source_url);

        /* fixup locations so they are easier to work with later */
        {
            char *location = strdup(sb->location);
            if (location == NULL)
                exit(EXIT_FAILURE);

            if (location[strlen(location) - 1] != '/') {
                char *fixed = add_part_to_url(location, "/");
                free(location);
                location = fixed;
            }

            if (strncmp(location, "./", 2) == 0) {
                char *fixed = strdup(location + 2);
                free(location);
                location = fixed;
            }

            fprintf(f, "SLACKBUILD LOCATION: %s\n", location);
            free(location);
        }

        fprintf(f, "SLACKBUILD FILES: ");
        for (uint32_t c = 0; c < sb->files->size; c++) {
            if (c == (sb->files->size - 1))
                fprintf(f, "%s\n", (char *)sb->files->items[c]);
            else
                fprintf(f, "%s ", (char *)sb->files->items[c]);
        }

        fprintf(f, "SLACKBUILD VERSION: %s\n", sb->version);
        fprintf(f, "SLACKBUILD DOWNLOAD: %s\n", sb->download ? sb->download : "");
        fprintf(f, "SLACKBUILD DOWNLOAD_x86_64: %s\n", sb->download_x86_64 ? sb->download_x86_64 : "");
        fprintf(f, "SLACKBUILD MD5SUM: %s\n", sb->md5sum ? sb->md5sum : "");
        fprintf(f, "SLACKBUILD MD5SUM_x86_64: %s\n", sb->md5sum_x86_64 ? sb->md5sum_x86_64 : "");
        fprintf(f, "SLACKBUILD REQUIRES: %s\n", sb->requires ? sb->requires : "");
        fprintf(f, "SLACKBUILD SHORT DESCRIPTION: %s\n", sb->short_desc ? sb->short_desc : "");
        fprintf(f, "\n");
    }

    fclose(f);
}

static void slapt_src_gz_read_fail(const char *datafile)
{
    fprintf(stderr, gettext("Failed to read %s: empty or corrupt gzip\n"), datafile);
    fprintf(stderr, gettext("Run --update to fetch the slackbuild list first\n"));
    exit(EXIT_FAILURE);
}

slapt_vector_t *slapt_src_get_slackbuilds_from_file(const char *datafile)
{
    slapt_vector_t *sbs = slapt_vector_t_init((slapt_vector_t_free_function)slapt_src_slackbuild_free);
    enum { SLAPT_SRC_NOT_PARSING,
           SLAPT_SRC_PARSING } parse_state;
    parse_state = SLAPT_SRC_NOT_PARSING;

    /* support reading from gzip'd files */
    FILE *f = NULL;
    const size_t datafile_len = strlen(datafile);
    if (datafile_len > 3 && strcmp(datafile + datafile_len - 3, ".gz") == 0) {
        gzFile data = NULL;
        char gzbuffer[SLAPT_MAX_ZLIB_BUFFER];

        if ((f = tmpfile()) == NULL)
            exit(EXIT_FAILURE);

        if ((data = gzopen(datafile, "rb")) == NULL) {
            fprintf(stderr, gettext("Failed to open %s for reading\n"), datafile);
            exit(EXIT_FAILURE);
        }

        FILE *magic_file = fopen(datafile, "rb");
        if (magic_file == NULL ||
            fgetc(magic_file) != 0x1f || fgetc(magic_file) != 0x8b) {
            if (magic_file != NULL)
                fclose(magic_file);
            slapt_src_gz_read_fail(datafile);
        }
        fclose(magic_file);

        bool got_data = false;
        while (gzgets(data, gzbuffer, SLAPT_MAX_ZLIB_BUFFER) != Z_NULL) {
            got_data = true;
            fprintf(f, "%s", gzbuffer);
        }
        const char *gz_err = gzerror(data, NULL);
        gzclose(data);
        rewind(f);

        if (!got_data || (gz_err != Z_NULL && gz_err[0] != '\0')) {
            slapt_src_gz_read_fail(datafile);
        }

    } else {
        f = fopen(datafile, "r");
        if (f == NULL) {
            fprintf(stderr, gettext("Failed to open %s for reading\n"), datafile);
            fprintf(stderr, gettext("Run --update to fetch the slackbuild list first\n"));
            exit(EXIT_FAILURE);
        }
    }

    slapt_src_slackbuild *sb = NULL;
    char *buffer = NULL;
    size_t gb_length = 0;
    ssize_t g_size;
    while ((g_size = getline(&buffer, &gb_length, f)) != EOF) {
        char *token = NULL;
        const int name_line = strstr(buffer, "SLACKBUILD NAME:") != NULL;

        /* a entry ends at a blank line or at the next NAME line; keep the last entry only if it is complete. */
        if (parse_state == SLAPT_SRC_PARSING &&
            (name_line || strcmp(buffer, "\n") == 0)) {
            if (sb->name == NULL || sb->name[0] == '\0' ||
                sb->location == NULL ||
                sb->version == NULL || sb->version[0] == '\0') {
                slapt_src_slackbuild_free(sb);
            } else {
                slapt_vector_t_add(sbs, sb);
            }
            sb = NULL;
            parse_state = SLAPT_SRC_NOT_PARSING;
        }

        if (name_line) {
            sb = slapt_src_slackbuild_init();
            parse_state = SLAPT_SRC_PARSING;
        }

        switch (parse_state) {
        case SLAPT_SRC_PARSING:

            /* we skip empty fields for odd sscanf bug in older glibcs */
            if (strstr(buffer, ": \n") != NULL)
                continue;

            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wformat"
            if ((sscanf(buffer, "SLACKBUILD NAME: %ms", &token)) == 1) {
            #pragma GCC diagnostic pop
                sb->name = strdup(token);
                free(token);
                if (!sb_name_valid(sb->name)) {
                    slapt_src_slackbuild_free(sb);
                    sb = NULL;
                    parse_state = SLAPT_SRC_NOT_PARSING;
                }
            }

            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wformat"
            if ((sscanf(buffer, "SLACKBUILD SOURCEURL: %ms", &token)) == 1) {
            #pragma GCC diagnostic pop
                sb->sb_source_url = strdup(token);
                free(token);
            }

            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wformat"
            if ((sscanf(buffer, "SLACKBUILD LOCATION: %ms", &token)) == 1) {
            #pragma GCC diagnostic pop
                sb->location = strdup(token);
                free(token);
                if (!sb_path_valid(sb->location)) {
                    slapt_src_slackbuild_free(sb);
                    sb = NULL;
                    parse_state = SLAPT_SRC_NOT_PARSING;
                }
            }

            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wformat"
            if ((sscanf(buffer, "SLACKBUILD FILES: %m[^\r\n]", &token)) == 1) {
            #pragma GCC diagnostic pop
                slapt_vector_t *files = slapt_parse_delimited_list(token, ' ');
                bool files_valid = true;
                slapt_vector_t_foreach(const char *, file, files) {
                    if (!sb_path_valid(file))
                        files_valid = false;
                    slapt_vector_t_add(sb->files, strdup(file));
                }
                slapt_vector_t_free(files);
                free(token);
                if (!files_valid) {
                    slapt_src_slackbuild_free(sb);
                    sb = NULL;
                    parse_state = SLAPT_SRC_NOT_PARSING;
                }
            }

            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wformat"
            if ((sscanf(buffer, "SLACKBUILD VERSION: %m[^\r\n]", &token)) == 1) {
            #pragma GCC diagnostic pop
                sb->version = strdup(token);
                free(token);
            }

            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wformat"
            if ((sscanf(buffer, "SLACKBUILD DOWNLOAD: %m[^\r\n]", &token)) == 1) {
            #pragma GCC diagnostic pop
                sb->download = strdup(token);
                free(token);
            }

            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wformat"
            if ((sscanf(buffer, "SLACKBUILD DOWNLOAD_x86_64: %m[^\r\n]", &token)) == 1) {
            #pragma GCC diagnostic pop
                sb->download_x86_64 = strdup(token);
                free(token);
            }

            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wformat"
            if ((sscanf(buffer, "SLACKBUILD MD5SUM: %m[^\r\n]", &token)) == 1) {
            #pragma GCC diagnostic pop
                sb->md5sum = strdup(token);
                free(token);
            }

            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wformat"
            if ((sscanf(buffer, "SLACKBUILD MD5SUM_x86_64: %m[^\r\n]", &token)) == 1) {
            #pragma GCC diagnostic pop
                sb->md5sum_x86_64 = strdup(token);
                free(token);
            }

            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wformat"
            if ((sscanf(buffer, "SLACKBUILD REQUIRES: %m[^\r\n]", &token)) == 1) {
            #pragma GCC diagnostic pop
                sb->requires = strdup(token);
                free(token);
            }

            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wformat"
            if ((sscanf(buffer, "SLACKBUILD SHORT DESCRIPTION: %m[^\r\n]", &token)) == 1) {
            #pragma GCC diagnostic pop
                sb->short_desc = strdup(token);
                free(token);
            }

            break;

        default:
            break;
        }
    }

    /* again, keep the last entry only if it is complete. */
    if (parse_state == SLAPT_SRC_PARSING) {
        if (sb->name == NULL || sb->name[0] == '\0' ||
            sb->location == NULL ||
            sb->version == NULL || sb->version[0] == '\0') {
            slapt_src_slackbuild_free(sb);
        } else {
            slapt_vector_t_add(sbs, sb);
        }
        sb = NULL;
        parse_state = SLAPT_SRC_NOT_PARSING;
    }

    if (buffer != NULL)
        free(buffer);

    fclose(f);

    slapt_vector_t_sort(sbs, sb_cmp);
    sbs->sorted = true;
    return sbs;
}

slapt_vector_t *slapt_src_get_available_slackbuilds(const char *builddir)
{
    char *datafile = slapt_gen_abs_path(builddir, SLAPT_SRC_DATA_FILE);
    slapt_vector_t *sbs = slapt_src_get_slackbuilds_from_file(datafile);
    free(datafile);
    return sbs;
}

static bool sb_chars_allowed(const char *name)
{
    if (name == NULL || name[0] == '\0')
        return false;

    for (const char *p = name; *p; p++) {
        if (!isalnum((unsigned char)*p) && strchr("._+-", (unsigned char)*p) == NULL)
            return false;
    }

    return true;
}

static bool sb_name_valid(const char *name)
{
    if (name == NULL || name[0] == '.' || strstr(name, "..") != NULL)
        return false;

    return sb_chars_allowed(name);
}

static bool sb_path_valid(const char *path)
{
    if (path == NULL || path[0] == '\0' || path[0] == '/')
        return false;

    char *copy = strdup(path);
    char *saveptr = NULL;
    for (const char *comp = strtok_r(copy, "/", &saveptr);
         comp != NULL; comp = strtok_r(NULL, "/", &saveptr)) {
        if (strcmp(comp, "..") == 0 || !sb_chars_allowed(comp)) {
            free(copy);
            return false;
        }
    }

    free(copy);
    return true;
}

static char *filename_from_url(const char *url)
{
    char *filename = NULL;
    slapt_vector_t *parts = slapt_parse_delimited_list(url, '/');
    if (parts->size > 0) {
        filename = strdup(parts->items[parts->size - 1]);
    }
    slapt_vector_t_free(parts);

    return filename;
}

static char *add_part_to_url(const char *url, const char *part)
{
    char *new = slapt_malloc(sizeof *new *(strlen(url) + strlen(part) + 1));
    strcpy(new, url);
    strcat(new, part);
    return new;
}

bool slapt_src_fetch_slackbuild(const slapt_src_config *config, const slapt_src_slackbuild *sb)
{
    bool rv = true;

    /* need to mkdir and chdir to sb->location */
    slapt_create_dir_structure(sb->location);
    if (chdir(sb->location) != 0) {
        fprintf(stderr, gettext("Failed to chdir to %s\n"), sb->location);
        exit(EXIT_FAILURE);
    }

    /* download slackbuild files */
    slapt_config_t *slapt_config = slapt_config_t_init();
    if (strlen(config->builddir) >= sizeof slapt_config->working_dir) {
        fprintf(stderr, gettext("BUILDDIR too long\n"));
        exit(EXIT_FAILURE);
    }
    slapt_strlcpy(slapt_config->working_dir, config->builddir, sizeof slapt_config->working_dir);

    char *sb_location = add_part_to_url(sb->sb_source_url, sb->location);
    slapt_vector_t_foreach(const char *, sb_file, sb->files) {
        const char *s = NULL;
        FILE *f = NULL;
        char *url = add_part_to_url(sb_location, sb_file);

        /* some files contain paths, create as necessary */
        if ((s = rindex(sb_file, '/')) != NULL) {
            char *initial_dir = strndup(sb_file, strlen(sb_file) - strlen(s) + 1);
            if (initial_dir != NULL) {
                slapt_create_dir_structure(initial_dir);
                free(initial_dir);
            }
        }
        f = slapt_open_file(sb_file, "w+b");

        if (f == NULL) {
            perror("Cannot open file for writing");
            exit(EXIT_FAILURE);
        }

        /* TODO support file resume */
        printf(gettext("Fetching %s..."), sb_file);
        const int curl_rv = slapt_download_data(f, url, 0, NULL, slapt_config);
        if (curl_rv == 0) {
            printf(gettext("Done\n"));
        } else {
            fprintf(stderr, gettext("Failed\n"));
            exit(EXIT_FAILURE);
        }

        free(url);
        fclose(f);
    }

    /* fetch download || download_x86_64 */
    slapt_vector_t *download_parts = NULL, *md5sum_parts = NULL;
    if (strcmp(uname_v.machine, "x86_64") == 0 && sb->download_x86_64 != NULL && strcmp(sb->download_x86_64, "") != 0 && strcmp(sb->download_x86_64, "UNSUPPORTED") != 0 && strcmp(sb->download_x86_64, "UNTESTED") != 0) {
        download_parts = slapt_parse_delimited_list(sb->download_x86_64, ' ');
        md5sum_parts = slapt_parse_delimited_list(sb->md5sum_x86_64, ' ');
    } else {
        if (sb->download != NULL)
            download_parts = slapt_parse_delimited_list(sb->download, ' ');
        else
            download_parts = slapt_vector_t_init(free); /* no download files */

        if (sb->md5sum != NULL)
            md5sum_parts = slapt_parse_delimited_list(sb->md5sum, ' ');
        else
            md5sum_parts = slapt_vector_t_init(free); /* no md5sum files */
    }

    if (download_parts == NULL || download_parts->size != md5sum_parts->size) {
        fprintf(stderr, gettext("Mismatch between download files and md5sums\n"));
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < download_parts->size; i++) {
        const char *md5sum = md5sum_parts->items[i];

        char *filename = filename_from_url(download_parts->items[i]);
        if (filename == NULL) {
            fprintf(stderr, gettext("Invalid download URL: %s\n"), (const char *)download_parts->items[i]);
            exit(EXIT_FAILURE);
        }
        struct stat file_stat;
        size_t file_size = 0;
        if (stat(filename, &file_stat) == 0) {
            /* if already exists, find out how much we already have */
            file_size = (size_t)file_stat.st_size;
        }

        FILE *f = slapt_open_file(filename, "a+b");
        if (f == NULL) {
            perror("Cannot open file for writing");
            exit(EXIT_FAILURE);
        }

        /* check checksum to see if we need to continue */
        char md5sum_to_prove[SLAPT_MD5_STR_LEN + 1];
        slapt_gen_md5_sum_of_file(f, md5sum_to_prove);
        if (strcmp(md5sum_to_prove, md5sum) != 0) {
            printf(gettext("Fetching %s..."), (char *)download_parts->items[i]);
            /* download/resume */
            const int curl_rv = slapt_download_data(f, download_parts->items[i], file_size, NULL, slapt_config);
            if (curl_rv == 0) {
                printf(gettext("Done\n"));
            } else {
                fprintf(stderr, gettext("Failed\n"));
                exit(EXIT_FAILURE);
            }

            /* verify checksum of downloaded file */
            slapt_gen_md5_sum_of_file(f, md5sum_to_prove);
            if (strcmp(md5sum_to_prove, md5sum) != 0) {
                fprintf(stderr, gettext("MD5SUM mismatch for %s\n"), filename);
                exit(EXIT_FAILURE);
            }
        }

        fclose(f);
        free(filename);
    }

    slapt_vector_t_free(download_parts);
    if (md5sum_parts != NULL)
        slapt_vector_t_free(md5sum_parts);

    slapt_config_t_free(slapt_config);
    free(sb_location);

    /* maybe show the README here */
    if (sb->requires && strstr(sb->requires, "%README%") != NULL) {
        printf("%%README%%\n");
        FILE *readme = slapt_open_file("README", "r");
        if (readme == NULL) {
            fprintf(stderr, gettext("Failed to open the README for %s\n"), sb->name);
            exit(EXIT_FAILURE);
        }

        ssize_t read_size;
        size_t read_len = 0;
        char *buffer = NULL;
        while ((read_size = getline(&buffer, &read_len, readme)) != EOF) {
            buffer[strcspn(buffer, "\r\n")] = '\0';
            printf("%s\n", buffer);
        }
        fclose(readme);
        if (buffer) {
            free(buffer);
        }
        if (config->prompt && slapt_ask_yes_no(gettext("Do you want to continue? [y/N] ")) != 1) {
            rv = false;
        }
    }

    /* go back */
    if (chdir(config->builddir) != 0) {
        fprintf(stderr, gettext("Failed to chdir to %s\n"), config->builddir);
        exit(EXIT_FAILURE);
    }
    return rv;
}

/* reads directory listing of current directory for a package */
static char *_get_pkg_filename(const char *pkgtag)
{
    DIR *d = opendir(".");
    if (d == NULL) {
        fprintf(stderr, gettext("Failed to open current directory\n"));
        exit(EXIT_FAILURE);
    }

    slapt_regex_t *pkg_regex = slapt_regex_t_init(SLAPT_PKG_PARSE_REGEX);
    if (pkg_regex == NULL) {
        exit(EXIT_FAILURE);
    }

    char *filename = NULL;
    struct dirent *file = NULL;
    while ((file = readdir(d)) != NULL) {
        if (strcmp(file->d_name, "..") == 0 || strcmp(file->d_name, ".") == 0)
            continue;

        struct stat stat_buf;
        if (lstat(file->d_name, &stat_buf) == -1) {
            continue;
        }
        if (!S_ISREG(stat_buf.st_mode)) {
            continue;
        }
        slapt_regex_t_execute(pkg_regex, file->d_name);
        if (pkg_regex->reg_return != 0) {
            continue;
        }

        /* version from .info file may not match resulting pkg version so we only look at the tag */
        if (pkgtag != NULL) {
            char *ver_arch_tag = slapt_regex_t_extract_match(pkg_regex, file->d_name, 2);
            if (ver_arch_tag != NULL) {
                const char *tag_sep = strrchr(ver_arch_tag, '-');
                if (tag_sep == NULL || strcmp(tag_sep + 1, pkgtag) != 0) {
                    free(ver_arch_tag);
                    continue; /* wrong tag */
                }
                free(ver_arch_tag);
            }
        }

        filename = strdup(file->d_name);
        break;
    }

    closedir(d);
    slapt_regex_t_free(pkg_regex);

    return filename;
}

/* slackbuild scripts rely on bash constructs (trap ERR, [[ ]]), which the
 * /bin/sh of a non-slackware host may reject, so prefer bash over sh when
 * it is present and runnable */
static const char *slapt_src_script_runner(void)
{
    struct stat bash_st;

    if (stat("/bin/bash", &bash_st) == 0 && S_ISREG(bash_st.st_mode)
        && (bash_st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0) {
        return "/bin/bash";
    }
    return "sh";
}

bool slapt_src_build_slackbuild(const slapt_src_config *config, const slapt_src_slackbuild *sb)
{
    if (chdir(sb->location) != 0) {
        fprintf(stderr, gettext("Failed to chdir to %s\n"), sb->location);
        exit(EXIT_FAILURE);
    }

    /* make sure we set locations and other env vars for the slackbuilds to honor */
    char *cwd = get_current_dir_name();
    setenv("TMP", cwd, 1);
    setenv("OUTPUT", cwd, 1);
    free(cwd);

    if (config->pkgext != NULL)
        setenv("PKGTYPE", config->pkgext, 1);

    if (config->pkgtag != NULL)
        setenv("TAG", config->pkgtag, 1);

    if (strcmp(uname_v.machine, "x86_64") == 0 && sb->download_x86_64 != NULL && strcmp(sb->download_x86_64, "") != 0 && strcmp(sb->download_x86_64, "UNSUPPORTED") != 0 && strcmp(sb->download_x86_64, "UNTESTED") != 0) {
        setenv("ARCH", uname_v.machine, 1);
    }

    /* run the script via fork/exec, never through a shell, so the
     * package name cannot be interpreted as shell syntax */
    char *script = NULL;
    const size_t script_len = strlen(sb->name) + strlen(".SlackBuild") + 1;
    script = slapt_malloc(sizeof *script * script_len);
    const int script_r = snprintf(script, script_len, "%s.SlackBuild", sb->name);
    if (script_r <= 0 || (size_t)script_r + 1 != script_len) {
        fprintf(stderr, gettext("Failed to construct script name\n"));
        free(script);
        exit(EXIT_FAILURE);
    }

#if defined(HAS_SLKBUILD)
    slapt_vector_t *slkbuild_matches = slapt_vector_t_search(sb->files, sb_compare_name_to_name, "SLKBUILD");
    const bool use_slkbuild = (slkbuild_matches != NULL);
    if (slkbuild_matches)
        slapt_vector_t_free(slkbuild_matches);
#else
    const bool use_slkbuild = false;
#endif

    /* argv for the script runner: [fakeroot --] sh <script> or [fakeroot --] slkbuild -X */
    char *build_argv[5];
    int build_argc = 0;
    if (use_slkbuild) {
#ifdef HAS_FAKEROOT
        build_argv[build_argc++] = (char *)"fakeroot";
#endif
        build_argv[build_argc++] = (char *)"slkbuild";
        build_argv[build_argc++] = (char *)"-X";
    } else {
#ifdef HAS_FAKEROOT
        build_argv[build_argc++] = (char *)"fakeroot";
        build_argv[build_argc++] = (char *)"--";
#endif
        build_argv[build_argc++] = (char *) slapt_src_script_runner();
        build_argv[build_argc++] = script;
    }
    build_argv[build_argc] = NULL;

    /* sb_flush() drops version-less entries, so sb->version is set */
    setenv("VERSION", sb->version, 1);
    const int r = slapt_src_exec(build_argv[0], build_argv);
    unsetenv("VERSION");
    unsetenv("TMP");
    unsetenv("OUTPUT");
    unsetenv("PKGTYPE");
    unsetenv("TAG");
    unsetenv("ARCH");
    if (r != 0) {
        /* report the full command so the runner (sh/fakeroot/slkbuild)
           is visible on failure, not just the script name */
        char cmd[512] = {0};
        size_t off = 0;
        for (int a = 0; a < build_argc; a++) {
            const int w = snprintf(cmd + off, sizeof(cmd) - off, "%s%s", a > 0 ? " " : "", build_argv[a]);
            if (w < 0 || (size_t)w >= sizeof(cmd) - off)
                break;
            off += (size_t)w;
        }
        fprintf(stderr, "%.*s %s\n", (int)off, cmd, gettext("Failed\n"));
        exit(EXIT_FAILURE);
    }

    free(script);

    if (config->postcmd != NULL) {
        char *filename = NULL;
        if ((filename = _get_pkg_filename(config->pkgtag)) != NULL) {
            char *postcmd_copy = strdup(config->postcmd);
            char *prog = postcmd_copy;
            char *arg = filename;
            char *const argv[] = { prog, arg, NULL };
            const int post_r = slapt_src_exec(prog, argv);
            if (post_r != 0) {
                fprintf(stderr, "%s %s %s\n", prog, arg, gettext("Failed\n"));
                free(postcmd_copy);
                free(filename);
                exit(EXIT_FAILURE);
            }
            free(postcmd_copy);
            free(filename);
        } else {
            fprintf(stderr, gettext("Unable to find generated package\n"));
            exit(EXIT_FAILURE);
        }
    }

    /* go back */
    if (chdir(config->builddir) != 0) {
        fprintf(stderr, gettext("Failed to chdir to %s\n"), config->builddir);
        exit(EXIT_FAILURE);
    }

    return true;
}

bool slapt_src_install_slackbuild(const slapt_src_config *config, const slapt_src_slackbuild *sb)
{
    if (chdir(sb->location) != 0) {
        fprintf(stderr, gettext("Failed to chdir to %s\n"), sb->location);
        exit(EXIT_FAILURE);
    }

    char *filename = _get_pkg_filename(config->pkgtag);
    if (filename != NULL) {
        char *const argv[] = { (char *)"/sbin/upgradepkg", (char *)"--reinstall", (char *)"--install-new", filename, NULL };
        const int r = slapt_src_exec("/sbin/upgradepkg", argv);
        if (r != 0) {
            fprintf(stderr, "/sbin/upgradepkg --reinstall --install-new %s %s\n", filename, gettext("Failed\n"));
            exit(EXIT_FAILURE);
        }

        free(filename);
    } else {
        fprintf(stderr, gettext("Unable to find generated package\n"));
        exit(EXIT_FAILURE);
    }

    /* go back */
    if (chdir(config->builddir) != 0) {
        fprintf(stderr, gettext("Failed to chdir to %s\n"), config->builddir);
        exit(EXIT_FAILURE);
    }
    return true;
}

slapt_src_slackbuild *slapt_src_get_slackbuild(const slapt_vector_t *sbs, const char *name, const char *version)
{
    if (sbs->size < 1) {
        return NULL;
    }
    int min = 0, max = (int)(sbs->size - 1);

    while (max >= min) {
        int pivot = (min + max) / 2;
        int name_cmp = strcmp(((slapt_src_slackbuild *)sbs->items[pivot])->name, name);

        if (name_cmp == 0) {
            if (version == NULL)
                return sbs->items[pivot];

            int ver_cmp = slapt_pkg_t_cmp_versions(((slapt_src_slackbuild *)sbs->items[pivot])->version, version);
            if (ver_cmp == 0)
                return sbs->items[pivot];

            if (ver_cmp < 0)
                min = pivot + 1;
            else
                max = pivot - 1;

        } else {
            if (name_cmp < 0)
                min = pivot + 1;
            else
                max = pivot - 1;
        }
    }

    return NULL;
}

static bool slapt_src_resolve_dependencies(
    const slapt_vector_t *available,
    const slapt_src_slackbuild *sb,
    slapt_vector_t *deps,
    const slapt_vector_t *installed,
    slapt_vector_t *errors,
    slapt_vector_t *visiting)
{
    slapt_vector_t *requires = NULL;
    if (sb->requires == NULL) {
        return true;
    }

    if (strstr(sb->requires, ",") != NULL) {
        requires = slapt_parse_delimited_list(sb->requires, ',');
    } else {
        requires = slapt_parse_delimited_list(sb->requires, ' ');
    }

    if (requires == NULL) {
        return true;
    }

    slapt_vector_t_foreach(const char *, dep_name, requires) {
        slapt_src_slackbuild *sb_dep = NULL;

        /* skip non-deps */
        if (strcmp(dep_name, "%README%") == 0) {
            continue;
        }

        sb_dep = slapt_src_get_slackbuild(available, dep_name, NULL);

        /* we will try and resolve its dependencies no matter what,
       in case there are new deps we don't yet have */
        if (sb_dep != NULL) {
            slapt_vector_t *matches = slapt_vector_t_search(deps, sb_compare_pkg_to_name, (char *)dep_name);
            if (matches) {
                slapt_vector_t_free(matches);
                continue;
            }

            slapt_vector_t *in_progress =
                slapt_vector_t_search(visiting, sb_compare_pkg_to_name, (char *)dep_name);
            if (in_progress != NULL) {
                slapt_vector_t_free(in_progress);
                fprintf(stderr, gettext("Circular dependency: %s requires %s\n"), sb->name, dep_name);
                slapt_vector_t_free(requires);
                return false;
            }

            /* avoid reprocessing */
            slapt_vector_t_add(visiting, sb_dep);
            bool dep_check = slapt_src_resolve_dependencies(available, sb_dep, deps, installed, errors, visiting);
            if (dep_check)
                slapt_vector_t_remove(visiting, sb_dep);

            if (!dep_check) {
                slapt_vector_t_free(requires);
                return dep_check;
            }

            /* if not installed */
            if (slapt_get_newest_pkg(installed, dep_name) == NULL) {
                slapt_vector_t_add(deps, sb_dep);
            }
        }
        /* we don't have a slackbuild for it */
        else {
            /* if not installed, this is an error */
            if (slapt_get_newest_pkg(installed, dep_name) == NULL) {
                slapt_vector_t_add(errors, slapt_pkg_err_t_init(strdup(sb->name), strdup((char *)dep_name)));
                slapt_vector_t_free(requires);
                return false;
            }
        }
    }

    slapt_vector_t_free(requires);
    return true;
}

slapt_vector_t *slapt_src_names_to_slackbuilds(
    const slapt_src_config *config,
    const slapt_vector_t *available,
    const slapt_vector_t *names,
    const slapt_vector_t *installed)
{
    slapt_vector_t *sbs = slapt_vector_t_init(NULL);//(slapt_vector_t_free_function)slapt_src_slackbuild_free);
    slapt_vector_t *unknown = slapt_vector_t_init(free);

    for (uint32_t i = 0; i < names->size; i++) {
        slapt_src_slackbuild *sb = NULL;
        slapt_vector_t *parts = slapt_parse_delimited_list(names->items[i], ':');
        if (parts->size > 1) {
            sb = slapt_src_get_slackbuild(available, parts->items[0], parts->items[1]);
        } else {
            sb = slapt_src_get_slackbuild(available, names->items[i], NULL);
        }

        slapt_vector_t_free(parts);

        if (sb != NULL) {
            if (config->do_dep == true) {
                slapt_vector_t *deps = slapt_vector_t_init(NULL);
                slapt_vector_t *errors = slapt_vector_t_init((slapt_vector_t_free_function)slapt_pkg_err_t_free);
                slapt_vector_t *visiting = slapt_vector_t_init(NULL);
                bool dep_check = slapt_src_resolve_dependencies(available, sb, deps, installed, errors, visiting);

                if (!dep_check) {
                    slapt_vector_t_foreach(slapt_pkg_err_t *, err, errors) {
                        fprintf(stderr, gettext("Missing slackbuild: %s requires %s\n"), err->pkg, err->error);
                    }
                    slapt_vector_t_free(deps);
                    slapt_vector_t_free(errors);
                    slapt_vector_t_free(visiting);
                    continue;
                }

                slapt_vector_t_free(errors);

                slapt_vector_t_foreach(slapt_src_slackbuild *, dep, deps) {
                    slapt_vector_t *dep_matches = slapt_vector_t_search(sbs, sb_compare_pkg_to_name, dep->name);
                    if (!dep_matches) {
                        slapt_vector_t_add(sbs, dep);
                    } else {
                        slapt_vector_t_free(dep_matches);
                    }
                }

                slapt_vector_t_free(deps);
                slapt_vector_t_free(visiting);
            }

            slapt_vector_t *matches = slapt_vector_t_search(sbs, sb_compare_pkg_to_name, sb->name);
            if (!matches) {
                slapt_vector_t_add(sbs, sb);
            } else {
                slapt_vector_t_free(matches);
            }
        } else {
            slapt_vector_t_add(unknown, strdup(names->items[i]));
        }
    }

    if (unknown->size > 0) {
        slapt_vector_t_foreach(const char *, unknown_name, unknown) {
            fprintf(stderr, gettext("Unknown slackbuild: %s\n"), unknown_name);
        }
        slapt_vector_t_free(unknown);
        slapt_vector_t_free(sbs);
        return NULL;
    }

    slapt_vector_t_free(unknown);
    return sbs;
}

slapt_vector_t *slapt_src_search_slackbuild_cache(const slapt_vector_t *remote_sbs, const slapt_vector_t *names)
{
    slapt_vector_t *sbs = slapt_vector_t_init(NULL);

    slapt_vector_t_foreach(char *, sb_name, names) {
        slapt_regex_t *search_regex = slapt_regex_t_init(sb_name);
        if (search_regex == NULL) {
            continue;
        }

        slapt_vector_t_foreach(slapt_src_slackbuild *, remote_sb, remote_sbs) {
            if (strcmp(remote_sb->name, sb_name) == 0) {
                slapt_vector_t_add(sbs, remote_sb);
                continue;
            }

            slapt_regex_t_execute(search_regex, remote_sb->name);
            const int name_r = search_regex->reg_return;

            slapt_regex_t_execute(search_regex, remote_sb->location);
            const int location_r = search_regex->reg_return;

            int short_desc_r = -1;
            if (remote_sb->short_desc != NULL) {
                slapt_regex_t_execute(search_regex, remote_sb->short_desc);
                short_desc_r = search_regex->reg_return;
            }

            if (name_r == 0 || location_r == 0 || short_desc_r == 0)
                slapt_vector_t_add(sbs, remote_sb);
        }

        slapt_regex_t_free(search_regex);
    }

    return sbs;
}

int sb_compare_name_to_name(const void *a, const void *b)
{
    const char *name_a = (const char *)a;
    const char *name_b = (const char *)b;
    return strcmp(name_a, name_b);
}

int sb_compare_pkg_to_name(const void *a, const void *b)
{
    const slapt_src_slackbuild *sb = (const slapt_src_slackbuild *)a;
    const char *name = (const char *)b;
    return strcmp(sb->name, name);
}
