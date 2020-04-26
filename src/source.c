/*
 * Copyright (C) 2010-2020 Jason Woodward <woodwardj at jaos dot org>
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
#include "source.h"
#include "config.h"

#ifdef HAS_FAKEROOT
#define SLAPTSRC_CMD_LEN 27
#define SLAPTSRC_CMD "fakeroot -- sh"
#define SLAPTSRC_SLKBUILD_CMD_LEN 24
#define SLAPTSRC_SLKBUILD_CMD "fakeroot -- slkbuild -X"
#else
#define SLAPTSRC_CMD_LEN 15
#define SLAPTSRC_CMD "sh"
#define SLAPTSRC_SLKBUILD_CMD_LEN 12
#define SLAPTSRC_SLKBUILD_CMD "slkbuild -X"
#endif

extern struct utsname uname_v;

static char *filename_from_url(char *url);
static char *add_part_to_url(char *url, char *part);

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

slapt_src_config *slapt_src_read_config(const char *filename)
{
    FILE *rc = NULL;
    slapt_src_config *config = NULL;
    char *buffer = NULL;
    size_t gb_length = 0;
    ssize_t g_size;

    rc = slapt_open_file(filename, "r");
    if (rc == NULL)
        exit(EXIT_FAILURE);

    config = slapt_src_config_init();

    while ((g_size = getline(&buffer, &gb_length, rc)) != EOF) {
        char *token_ptr = NULL;
        buffer[g_size - 1] = '\0';

        if (strchr(buffer, '#') != NULL)
            continue;

        if ((token_ptr = strstr(buffer, SLAPT_SRC_SOURCE_TOKEN)) != NULL) {
            if (strlen(token_ptr) > strlen(SLAPT_SRC_SOURCE_TOKEN)) {
                char *source = strdup(token_ptr + strlen(SLAPT_SRC_SOURCE_TOKEN));
                if (source[strlen(source) - 1] != '/') {
                    char *fixed = add_part_to_url(source, "/");
                    free(source);
                    source = fixed;
                }
                slapt_vector_t_add(config->sources, strdup(source));
                free(source);
            }

        } else if ((token_ptr = strstr(buffer, SLAPT_SRC_BUILDDIR_TOKEN)) != NULL) {
            if (strlen(token_ptr) > strlen(SLAPT_SRC_BUILDDIR_TOKEN))
                config->builddir = strdup(token_ptr + strlen(SLAPT_SRC_BUILDDIR_TOKEN));

        } else if ((token_ptr = strstr(buffer, SLAPT_SRC_PKGEXT_TOKEN)) != NULL) {
            if (strlen(token_ptr) > strlen(SLAPT_SRC_PKGEXT_TOKEN))
                config->pkgext = strdup(token_ptr + strlen(SLAPT_SRC_PKGEXT_TOKEN));

        } else if ((token_ptr = strstr(buffer, SLAPT_SRC_PKGTAG_TOKEN)) != NULL) {
            if (strlen(token_ptr) > strlen(SLAPT_SRC_PKGTAG_TOKEN))
                config->pkgtag = strdup(token_ptr + strlen(SLAPT_SRC_PKGTAG_TOKEN));
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

bool slapt_src_update_slackbuild_cache(slapt_src_config *config)
{
    bool rval = true;
    slapt_config_t *slapt_config = slapt_config_t_init();
    slapt_vector_t *slackbuilds = slapt_vector_t_init((slapt_vector_t_free_function)slapt_src_slackbuild_free);

    slapt_vector_t_foreach(const char *, url, config->sources) {
        slapt_vector_t *sbs = NULL;
        char *files[] = {SLAPT_SRC_SOURCES_LIST_GZ, SLAPT_SRC_SOURCES_LIST, NULL};
        int fc;

        printf(gettext("Fetching slackbuild list from %s..."), url);

        for (fc = 0; files[fc] != NULL; fc++) {
            char *filename = NULL, *local_head = NULL, *head = NULL;
            const char *err = NULL;
            FILE *f = NULL;

            filename = slapt_gen_filename_from_url(url, files[fc]);
            local_head = slapt_read_head_cache(filename);
            head = slapt_head_mirror_data(url, files[fc]);

            /* is it cached ? */
            if (head != NULL && local_head != NULL && strcmp(head, local_head) == 0) {
                printf(gettext("Cached\n"));
                sbs = slapt_src_get_slackbuilds_from_file(filename);
            } else {
                if ((f = slapt_open_file(filename, "w+b")) == NULL)
                    exit(EXIT_FAILURE);

                err = slapt_get_mirror_data_from_source(f, slapt_config, url, files[fc]);
                fclose(f);

                if (!err) {
                    printf(gettext("Done\n"));
                    sbs = slapt_src_get_slackbuilds_from_file(filename);

                    if (head != NULL)
                        slapt_write_head_cache(head, filename);

                    free(head);
                    free(local_head);
                    free(filename);
                    break;

                } else {
                    slapt_clear_head_cache(filename);
                }
            }
            if (strcmp(files[fc], SLAPT_SRC_SOURCES_LIST_GZ) != 0) {
                if (err) {
                    fprintf(stderr, gettext("Download failed: %s\n"), err);
                } else {
                    fprintf(stderr, gettext("Download failed: %s\n"), "404");
                }
                rval = false;
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
        }
    }

    slapt_src_write_slackbuilds_to_file(slackbuilds, SLAPT_SRC_DATA_FILE);
    slapt_config_t_free(slapt_config);
    slapt_vector_t_free(slackbuilds);
    return rval;
}

static int sb_cmp(const void *a, const void *b)
{
    slapt_src_slackbuild *sb1 = *(slapt_src_slackbuild *const *)a;
    slapt_src_slackbuild *sb2 = *(slapt_src_slackbuild *const *)b;

    int cmp = strcmp(sb1->name, sb2->name);
    if (cmp != 0)
        return cmp;
    else
        return slapt_cmp_pkg_versions(sb1->version, sb2->version);
}

void slapt_src_write_slackbuilds_to_file(slapt_vector_t *sbs, const char *datafile)
{
    FILE *f = slapt_open_file(datafile, "w+b");

    slapt_vector_t_sort(sbs, sb_cmp);

    slapt_vector_t_foreach(slapt_src_slackbuild *, sb, sbs) {
        /* write out package data */
        fprintf(f, "SLACKBUILD NAME: %s\n", sb->name);
        fprintf(f, "SLACKBUILD SOURCEURL: %s\n", sb->sb_source_url);

        /* fixup locations so they are easier to work with later */
        {
            char *location = strdup(sb->location);

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

slapt_vector_t *slapt_src_get_slackbuilds_from_file(const char *datafile)
{
    FILE *f = NULL;
    char *buffer = NULL;
    size_t gb_length = 0;
    ssize_t g_size;
    slapt_vector_t *sbs = slapt_vector_t_init((slapt_vector_t_free_function)slapt_src_slackbuild_free);
    slapt_src_slackbuild *sb = NULL;
    enum { SLAPT_SRC_NOT_PARSING,
           SLAPT_SRC_PARSING } parse_state;
    parse_state = SLAPT_SRC_NOT_PARSING;

    /* support reading from gzip'd files */
    if (strstr(datafile, ".gz") != NULL) {
        gzFile data = NULL;
        char gzbuffer[SLAPT_MAX_ZLIB_BUFFER];

        if ((f = tmpfile()) == NULL)
            exit(EXIT_FAILURE);

        if ((data = gzopen(datafile, "rb")) == NULL)
            exit(EXIT_FAILURE);

        while (gzgets(data, gzbuffer, SLAPT_MAX_ZLIB_BUFFER) != Z_NULL) {
            fprintf(f, "%s", gzbuffer);
        }
        gzclose(data);
        rewind(f);

    } else {
        f = fopen(datafile, "r");
        if (f == NULL) {
            printf(gettext("Failed to open %s for reading\n"), datafile);
            return sbs;
        }
    }

    while ((g_size = getline(&buffer, &gb_length, f)) != EOF) {
        char *token = NULL;

        if (strstr(buffer, "SLACKBUILD NAME:") != NULL) {
            parse_state = SLAPT_SRC_PARSING;
            sb = slapt_src_slackbuild_init();
        }

        if ((strcmp(buffer, "\n") == 0) && (parse_state == SLAPT_SRC_PARSING)) {
            slapt_vector_t_add(sbs, sb);
            /* do not free, in use in sbs list
            slapt_src_slackbuild_free (sb);
            */
            sb = NULL;
            parse_state = SLAPT_SRC_NOT_PARSING;
        }

        switch (parse_state) {
        case SLAPT_SRC_PARSING:

            /* we skip empty fields for odd sscanf bug in older glibcs */
            if (strstr(buffer, ": \n") != NULL)
                continue;

            if ((sscanf(buffer, "SLACKBUILD NAME: %ms", &token)) == 1) {
                sb->name = strdup(token);
                free(token);
            }

            if ((sscanf(buffer, "SLACKBUILD SOURCEURL: %ms", &token)) == 1) {
                sb->sb_source_url = strdup(token);
                free(token);
            }

            if ((sscanf(buffer, "SLACKBUILD LOCATION: %ms", &token)) == 1) {
                sb->location = strdup(token);
                free(token);
            }

            if ((sscanf(buffer, "SLACKBUILD FILES: %m[^\n]", &token)) == 1) {
                slapt_vector_t *files = slapt_parse_delimited_list(token, ' ');
                slapt_vector_t_foreach(const char *, file, files) {
                    slapt_vector_t_add(sb->files, strdup(file));
                }
                slapt_vector_t_free(files);
                free(token);
            }

            if ((sscanf(buffer, "SLACKBUILD VERSION: %m[^\n]", &token)) == 1) {
                sb->version = strdup(token);
                free(token);
            }

            if ((sscanf(buffer, "SLACKBUILD DOWNLOAD: %m[^\n]", &token)) == 1) {
                sb->download = strdup(token);
                free(token);
            }

            if ((sscanf(buffer, "SLACKBUILD DOWNLOAD_x86_64: %m[^\n]", &token)) == 1) {
                sb->download_x86_64 = strdup(token);
                free(token);
            }

            if ((sscanf(buffer, "SLACKBUILD MD5SUM: %m[^\n]", &token)) == 1) {
                sb->md5sum = strdup(token);
                free(token);
            }

            if ((sscanf(buffer, "SLACKBUILD MD5SUM_x86_64: %m[^\n]", &token)) == 1) {
                sb->md5sum_x86_64 = strdup(token);
                free(token);
            }

            if ((sscanf(buffer, "SLACKBUILD REQUIRES: %m[^\n]", &token)) == 1) {
                sb->requires = strdup(token);
                free(token);
            }

            if ((sscanf(buffer, "SLACKBUILD SHORT DESCRIPTION: %m[^\n]", &token)) == 1) {
                sb->short_desc = strdup(token);
                free(token);
            }

            break;

        default:
            break;
        }
    }

    if (buffer != NULL)
        free(buffer);

    fclose(f);

    sbs->sorted = true;
    return sbs;
}

slapt_vector_t *slapt_src_get_available_slackbuilds()
{
    return slapt_src_get_slackbuilds_from_file(SLAPT_SRC_DATA_FILE);
}

static char *filename_from_url(char *url)
{
    char *filename = NULL;
    slapt_vector_t *parts = slapt_parse_delimited_list(url, '/');
    if (parts->size > 0) {
        filename = strdup(parts->items[parts->size - 1]);
    }
    slapt_vector_t_free(parts);

    return filename;
}

static char *add_part_to_url(char *url, char *part)
{
    char *new = malloc(sizeof *new *(strlen(url) + strlen(part) + 1));
    if (new != NULL) {
        new = strcpy(new, url);
        new = strcat(new, part);
    }

    return new;
}

bool slapt_src_fetch_slackbuild(slapt_src_config *config, slapt_src_slackbuild *sb)
{
    slapt_vector_t *download_parts = NULL, *md5sum_parts = NULL;
    slapt_config_t *slapt_config = slapt_config_t_init();
    char *sb_location = add_part_to_url(sb->sb_source_url, sb->location);
    bool rv = true;

    /* need to mkdir and chdir to sb->location */
    slapt_create_dir_structure(sb->location);
    if (chdir(sb->location) != 0) {
        printf(gettext("Failed to chdir to %s\n"), sb->location);
        exit(EXIT_FAILURE);
    }

    /* download slackbuild files */
    slapt_vector_t_foreach(char *, sb_file, sb->files) {
        int curl_rv = 0;
        char *s = NULL, *url = add_part_to_url(sb_location, sb_file);
        FILE *f = NULL;

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
        curl_rv = slapt_download_data(f, url, 0, NULL, slapt_config);
        if (curl_rv == 0) {
            printf(gettext("Done\n"));
        } else {
            printf(gettext("Failed\n"));
            exit(EXIT_FAILURE);
        }

        free(url);
        fclose(f);
    }

    /* fetch download || download_x86_64 */
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

    if (download_parts->size != md5sum_parts->size) {
        printf(gettext("Mismatch between download files and md5sums\n"));
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < download_parts->size; i++) {
        int curl_rv = 0;
        char *md5sum = md5sum_parts->items[i];
        char *filename = filename_from_url(download_parts->items[i]);
        FILE *f = NULL;
        char md5sum_to_prove[SLAPT_MD5_STR_LEN + 1];
        struct stat file_stat;
        size_t file_size = 0;

        if (stat(filename, &file_stat) == 0) {
            /* if already exists, find out how much we already have */
            file_size = file_stat.st_size;
        }

        f = slapt_open_file(filename, "a+b");
        if (f == NULL) {
            perror("Cannot open file for writing");
            exit(EXIT_FAILURE);
        }

        /* check checksum to see if we need to continue */
        slapt_gen_md5_sum_of_file(f, md5sum_to_prove);
        if (strcmp(md5sum_to_prove, md5sum) != 0) {
            printf(gettext("Fetching %s..."), (char *)download_parts->items[i]);
            /* download/resume */
            curl_rv = slapt_download_data(f, download_parts->items[i], file_size, NULL, slapt_config);
            if (curl_rv == 0) {
                printf(gettext("Done\n"));
            } else {
                printf(gettext("Failed\n"));
                exit(EXIT_FAILURE);
            }

            /* verify checksum of downloaded file */
            slapt_gen_md5_sum_of_file(f, md5sum_to_prove);
            if (strcmp(md5sum_to_prove, md5sum) != 0) {
                printf(gettext("MD5SUM mismatch for %s\n"), filename);
                exit(EXIT_FAILURE);
            }
        }

        fclose(f);
        free(filename);
    }

    if (download_parts != NULL)
        slapt_vector_t_free(download_parts);
    if (md5sum_parts != NULL)
        slapt_vector_t_free(md5sum_parts);

    slapt_config_t_free(slapt_config);
    free(sb_location);

    /* maybe show the README here */
    if (sb->requires && strstr(sb->requires, "%README%") != NULL) {
        printf("%%README%%\n");
        FILE *readme = slapt_open_file("README", "r");
        if (readme == NULL)
            exit(EXIT_FAILURE);
        ssize_t read_size;
        size_t read_len = 0;
        char *buffer = NULL;
        while ((read_size = getline(&buffer, &read_len, readme)) != EOF) {
            buffer[read_size - 1] = '\0';
            printf("%s\n", buffer);
        }
        if (buffer)
            free(buffer);
        if (config->prompt && slapt_ask_yes_no(gettext("Do you want to continue? [y/N] ")) != 1) {
            rv = false;
        }
    }

    /* go back */
    if (chdir(config->builddir) != 0) {
        printf(gettext("Failed to chdir to %s\n"), config->builddir);
        exit(EXIT_FAILURE);
    }
    return rv;
}

/* reads directory listing of current directory for a package */
static char *_get_pkg_filename(const char *version, const char *pkgtag)
{
    char *filename = NULL;
    DIR *d = NULL;
    struct dirent *file = NULL;
    struct stat stat_buf;
    slapt_regex_t *pkg_regex = NULL;

    d = opendir(".");
    if (d == NULL) {
        printf(gettext("Failed to open current directory\n"));
        exit(EXIT_FAILURE);
    }

    if ((pkg_regex = slapt_regex_t_init(SLAPT_PKG_PARSE_REGEX)) == NULL) {
        exit(EXIT_FAILURE);
    }

    while ((file = readdir(d)) != NULL) {
        if (strcmp(file->d_name, "..") == 0 || strcmp(file->d_name, ".") == 0)
            continue;

        if (lstat(file->d_name, &stat_buf) == -1)
            continue;

        if (!S_ISREG(stat_buf.st_mode))
            continue;

        slapt_regex_t_execute(pkg_regex, file->d_name);
        if (pkg_regex->reg_return != 0)
            continue;

        /* version from .info file may not match resulting pkg version
	 * libva: VERSION="0.31.1-1+sds4" and VERSION=0.31.1_1+sds4
    	if (strstr (file->d_name, version) == NULL)
            continue;
        */
        (void)version;

        if (pkgtag != NULL)
            if (strstr(file->d_name, pkgtag) == NULL)
                continue;

        filename = strdup(file->d_name);
        break;
    }

    closedir(d);
    slapt_regex_t_free(pkg_regex);

    return filename;
}

bool slapt_src_build_slackbuild(slapt_src_config *config, slapt_src_slackbuild *sb)
{
    char *cwd = NULL;
    char *command = NULL;
    int command_len = SLAPTSRC_CMD_LEN, r = 0;

    if (chdir(sb->location) != 0) {
        printf(gettext("Failed to chdir to %s\n"), sb->location);
        exit(EXIT_FAILURE);
    }

    /*
    make sure we set locations and other env vars for the slackbuilds to honor
  */
    cwd = get_current_dir_name();
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

#if defined(HAS_SLKBUILD)
    slapt_vector_t *slkbuild_matches = slapt_vector_t_search(sb->files, sb_compare_name_to_name, "SLKBUILD");
    if (slkbuild_matches) {
        command_len = SLAPTSRC_SLKBUILD_CMD_LEN;
        command = slapt_malloc(sizeof *command * command_len);
        r = snprintf(command, command_len, "%s", SLAPTSRC_SLKBUILD_CMD);
        slapt_vector_t_free(slkbuild_matches);
    } else {
#endif
        command_len += strlen(sb->name);
        command = slapt_malloc(sizeof *command * command_len);
        r = snprintf(command, command_len, "%s %s.SlackBuild", SLAPTSRC_CMD, sb->name);
#if defined(HAS_SLKBUILD)
    }
#endif
    if (r + 1 != command_len) {
        printf("%s (%d,%d,%s)\n", gettext("Failed to construct command string\n"), r, command_len, command);
        exit(EXIT_FAILURE);
    }

    setenv("VERSION", sb->version, 1);
    r = system(command);
    unsetenv("VERSION");
    if (r != 0) {
        printf("%s %s\n", command, gettext("Failed\n"));
        exit(EXIT_FAILURE);
    }

    free(command);
    command = NULL;

    if (config->postcmd != NULL) {
        char *filename = NULL;
        if ((filename = _get_pkg_filename(sb->version, config->pkgtag)) != NULL) {
            r = 0;
            command_len = strlen(config->postcmd) + strlen(filename) + 2;
            command = slapt_malloc(sizeof *command * command_len);
            r = snprintf(command, command_len, "%s %s", config->postcmd, filename);
            if (r + 1 != command_len) {
                printf(gettext("Failed to construct command string\n"));
                exit(EXIT_FAILURE);
            }
            r = system(command);
            if (r != 0) {
                printf("%s %s\n", command, gettext("Failed\n"));
                exit(EXIT_FAILURE);
            }
            free(command);
        } else {
            printf(gettext("Unable to find generated package\n"));
            exit(EXIT_FAILURE);
        }
        free(filename);
    }

    /* go back */
    if (chdir(config->builddir) != 0) {
        printf(gettext("Failed to chdir to %s\n"), config->builddir);
        exit(EXIT_FAILURE);
    }

    return true;
}

bool slapt_src_install_slackbuild(slapt_src_config *config, slapt_src_slackbuild *sb)
{
    char *filename = NULL;
    char *command = NULL;
    int command_len = 44, r = 0;

    if (chdir(sb->location) != 0) {
        printf(gettext("Failed to chdir to %s\n"), sb->location);
        exit(EXIT_FAILURE);
    }

    if ((filename = _get_pkg_filename(sb->version, config->pkgtag)) != NULL) {
        command_len += strlen(filename);
        command = slapt_malloc(sizeof *command * command_len);
        r = snprintf(command, command_len, "/sbin/upgradepkg --reinstall --install-new %s", filename);
        if (r + 1 != command_len) {
            printf(gettext("Failed to construct command string\n"));
            exit(EXIT_FAILURE);
        }

        r = system(command);
        if (r != 0) {
            printf("%s %s\n", command, gettext("Failed\n"));
            exit(EXIT_FAILURE);
        }

        free(filename);
        free(command);
    } else {
        printf(gettext("Unable to find generated package\n"));
        exit(EXIT_FAILURE);
    }

    /* go back */
    if (chdir(config->builddir) != 0) {
        printf(gettext("Failed to chdir to %s\n"), config->builddir);
        exit(EXIT_FAILURE);
    }
    return true;
}

slapt_src_slackbuild *slapt_src_get_slackbuild(slapt_vector_t *sbs, const char *name, const char *version)
{
    int min = 0, max = sbs->size - 1;

    while (max >= min) {
        int pivot = (min + max) / 2;
        int name_cmp = strcmp(((slapt_src_slackbuild *)sbs->items[pivot])->name, name);
        int ver_cmp = 0;

        if (name_cmp == 0) {
            if (version == NULL)
                return sbs->items[pivot];

            ver_cmp = slapt_cmp_pkg_versions(((slapt_src_slackbuild *)sbs->items[pivot])->version, version);
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
    slapt_vector_t *available,
    slapt_src_slackbuild *sb,
    slapt_vector_t *deps,
    slapt_vector_t *installed,
    slapt_vector_t *errors)
{
    slapt_vector_t *requires = NULL;
    if (sb->requires == NULL)
        return true;

    if (strstr(sb->requires, ",") != NULL)
        requires = slapt_parse_delimited_list(sb->requires, ',');
    else
        requires = slapt_parse_delimited_list(sb->requires, ' ');

    if (requires == NULL)
        return true;

    slapt_vector_t_foreach(const char *, dep_name, requires) {
        slapt_src_slackbuild *sb_dep = NULL;

        /* skip non-deps */
        if (strcmp(dep_name, "%README%") == 0)
            continue;

        sb_dep = slapt_src_get_slackbuild(available, dep_name, NULL);

        /* we will try and resolve its dependencies no matter what,
       in case there are new deps we don't yet have */
        if (sb_dep != NULL) {
            slapt_vector_t *matches = slapt_vector_t_search(deps, sb_compare_pkg_to_name, (char *)dep_name);
            if (matches) {
                slapt_vector_t_free(matches);
                continue;
            }

            bool dep_check = slapt_src_resolve_dependencies(available, sb_dep, deps, installed, errors);

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
    slapt_src_config *config,
    slapt_vector_t *available,
    slapt_vector_t *names,
    slapt_vector_t *installed)
{
    slapt_vector_t *sbs = slapt_vector_t_init(NULL);//(slapt_vector_t_free_function)slapt_src_slackbuild_free);

    for (uint32_t i = 0; i < names->size; i++) {
        slapt_src_slackbuild *sb = NULL;
        slapt_vector_t *parts = slapt_parse_delimited_list(names->items[i], ':');
        if (parts->size > 1)
            sb = slapt_src_get_slackbuild(available, parts->items[0], parts->items[1]);
        else
            sb = slapt_src_get_slackbuild(available, names->items[i], NULL);

        slapt_vector_t_free(parts);

        if (sb != NULL) {
            if (config->do_dep == true) {
                slapt_vector_t *deps = slapt_vector_t_init(NULL);
                slapt_vector_t *errors = slapt_vector_t_init((slapt_vector_t_free_function)slapt_pkg_err_t_free);
                slapt_vector_t_add(deps, sb); /* mark self as dep to prevent recursion */
                bool dep_check = slapt_src_resolve_dependencies(available, sb, deps, installed, errors);

                if (!dep_check) {
                    slapt_vector_t_foreach(slapt_pkg_err_t *, err, errors) {
                        fprintf(stderr, gettext("Missing slackbuild: %s requires %s\n"), err->pkg, err->error);
                    }
                    slapt_vector_t_free(deps);
                    slapt_vector_t_free(errors);
                    continue;
                }

                slapt_vector_t_free(errors);

                slapt_vector_t_foreach(slapt_src_slackbuild *, dep, deps) {
                    if (strcmp(sb->name, dep->name) == 0) {
                        continue;
                    }

                    slapt_vector_t *dep_matches = slapt_vector_t_search(sbs, sb_compare_pkg_to_name, dep->name);
                    if (!dep_matches) {
                        slapt_vector_t_add(sbs, dep);
                    } else {
                        slapt_vector_t_free(dep_matches);
                    }
                }

                slapt_vector_t_free(deps);
            }

            slapt_vector_t *matches = slapt_vector_t_search(sbs, sb_compare_pkg_to_name, sb->name);
            if (!matches) {
                slapt_vector_t_add(sbs, sb);
            } else {
                slapt_vector_t_free(matches);
            }
        }
    }

    return sbs;
}

slapt_vector_t *slapt_src_search_slackbuild_cache(slapt_vector_t *remote_sbs, slapt_vector_t *names)
{
    slapt_vector_t *sbs = slapt_vector_t_init(NULL);

    slapt_vector_t_foreach(char *, sb_name, names) {
        slapt_regex_t *search_regex = slapt_regex_t_init(sb_name);
        if (search_regex == NULL)
            continue;

        slapt_vector_t_foreach(slapt_src_slackbuild *, remote_sb, remote_sbs) {
            int name_r = -1, version_r = -1, short_desc_r = -1;

            if (strcmp(remote_sb->name, sb_name) == 0) {
                slapt_vector_t_add(sbs, remote_sb);
                continue;
            }

            slapt_regex_t_execute(search_regex, remote_sb->name);
            name_r = search_regex->reg_return;

            slapt_regex_t_execute(search_regex, remote_sb->location);
            version_r = search_regex->reg_return;

            if (remote_sb->short_desc != NULL) {
                slapt_regex_t_execute(search_regex, remote_sb->short_desc);
                short_desc_r = search_regex->reg_return;
            }

            if (name_r == 0 || version_r == 0 || short_desc_r == 0)
                slapt_vector_t_add(sbs, remote_sb);
        }

        slapt_regex_t_free(search_regex);
    }

    return sbs;
}

int sb_compare_name_to_name(const void *a, const void *b)
{
    char *name_a = (char *)a;
    char *name_b = (char *)b;
    return strcmp(name_a, name_b);
}

int sb_compare_pkg_to_name(const void *a, const void *b)
{
    slapt_src_slackbuild *sb = (slapt_src_slackbuild *)a;
    char *name = (char *)b;
    return strcmp(sb->name, name);
}
