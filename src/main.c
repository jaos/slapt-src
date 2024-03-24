/*
 * Copyright (C) 2010-2023 Jason Woodward <woodwardj at jaos dot org>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <slapt.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <config.h>
#include "source.h"

#define BUILD_ONLY_FLAG 1
#define FETCH_ONLY_FLAG 2

static int show_summary(slapt_vector_t *, slapt_vector_t *, int, bool);
static void clean(slapt_src_config *config);

void version(void)
{
    printf(gettext("%s version %s\n"), PACKAGE, VERSION);
    printf("Jason Woodward <woodwardj at jaos dot org>\n");
    printf("\n");
    printf("This program is free software; you can redistribute it and/or modify\n");
    printf("it under the terms of the GNU General Public License as published by\n");
    printf("the Free Software Foundation; either version 2 of the License, or\n");
    printf("any later version.\n");
    printf("This program is distributed in the hope that it will be useful,\n");
    printf("\n");
    printf("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
    printf("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
    printf("GNU Library General Public License for more details.\n");
    printf("\n");
    printf("You should have received a copy of the GNU General Public License\n");
    printf("along with this program; if not, write to the Free Software\n");
    printf("Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n");
}

void help(void)
{
    printf(gettext("%s - A SlackBuild utility\n"), PACKAGE);
    printf(gettext("Usage: %s [option(s)] [action] [slackbuild(s)]\n"), PACKAGE);
    printf("  -u, --update           %s\n", gettext("update local cache of remote slackbuilds"));
    printf("  -U, --upgrade-all      %s\n", gettext("upgrade all installed slackbuilds"));
    printf("  -l, --list             %s\n", gettext("list available slackbuilds"));
    printf("  -e, --clean            %s\n", gettext("clean build directory"));
    printf("  -s, --search           %s\n", gettext("search available slackbuilds"));
    printf("  -w, --show             %s\n", gettext("show specified slackbuilds"));
    printf("  -i, --install          %s\n", gettext("fetch, build, and install the specified slackbuild(s)"));
    printf("  -b, --build            %s\n", gettext("only fetch and build the specified slackbuild(s)"));
    printf("  -f, --fetch            %s\n", gettext("only fetch the specified slackbuild(s)"));
    printf("  -v, --version\n");
    printf("  -h, --help\n");
    printf(" %s:\n", gettext("Options"));
    printf("  -y, --yes              %s\n", gettext("do not prompt"));
    printf("  -t, --simulate         %s\n", gettext("show what will be done"));
    printf("  -c, --config=FILE      %s\n", gettext("use the specified configuration file"));
    printf("  -n, --no-dep           %s\n", gettext("do not look for dependencies"));
    printf("  -p, --postprocess=CMD  %s\n", gettext("run specified command on generated package"));
    printf("  -B, --build-only       %s\n", gettext("applicable only to --upgrade-all"));
    printf("  -F, --fetch-only       %s\n", gettext("applicable only to --upgrade-all"));
    printf("  -S, --skip-installable %s\n", gettext("skip if available via slapt-get, applicable only to --upgrade-all"));
}

#define VERSION_OPT 'v'
#define HELP_OPT 'h'
#define UPDATE_OPT 'u'
#define UPGRADE_OPT 'U'
#define LIST_OPT 'l'
#define SEARCH_OPT 's'
#define SHOW_OPT 'w'
#define FETCH_OPT 'f'
#define BUILD_OPT 'b'
#define INSTALL_OPT 'i'
#define YES_OPT 'y'
#define CONFIG_OPT 'c'
#define NODEP_OPT 'n'
#define POSTCMD_OPT 'p'
#define CLEAN_OPT 'e'
#define SIMULATE_OPT 't'
#define BUILD_ONLY_OPT 'B'
#define FETCH_ONLY_OPT 'F'
#define SKIP_INSTALLABLE_PKGS_OPT 'S'

struct utsname uname_v; /* for .machine */

static void init_builddir(slapt_src_config *config)
{
    DIR *builddir = NULL;
    mode_t mode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

    if ((builddir = opendir(config->builddir)) == NULL) {
        if (mkdir(config->builddir, mode) == -1) {
            printf(gettext("Failed to create build directory [%s]\n"), config->builddir);

            if (errno)
                perror(config->builddir);

            exit(EXIT_FAILURE);
        }
    } else {
        closedir(builddir);
    }
}

int main(int argc, char *argv[])
{
    static struct option long_options[] = {
        {"build", required_argument, 0, BUILD_OPT},
        {"b", required_argument, 0, BUILD_OPT},
        {"build-only", no_argument, 0, BUILD_ONLY_OPT},
        {"B", no_argument, 0, BUILD_ONLY_OPT},
        {"clean", no_argument, 0, CLEAN_OPT},
        {"e", no_argument, 0, CLEAN_OPT},
        {"config", required_argument, 0, CONFIG_OPT},
        {"c", required_argument, 0, CONFIG_OPT},
        {"fetch", required_argument, 0, FETCH_OPT},
        {"f", required_argument, 0, FETCH_OPT},
        {"fetch-only", no_argument, 0, FETCH_ONLY_OPT},
        {"F", no_argument, 0, FETCH_ONLY_OPT},
        {"help", no_argument, 0, HELP_OPT},
        {"install", required_argument, 0, INSTALL_OPT},
        {"list", no_argument, 0, LIST_OPT},
        {"no-dep", no_argument, 0, NODEP_OPT},
        {"postprocess", required_argument, 0, POSTCMD_OPT},
        {"search", required_argument, 0, SEARCH_OPT},
        {"s", required_argument, 0, SEARCH_OPT},
        {"skip-installable", no_argument, 0, SKIP_INSTALLABLE_PKGS_OPT},
        {"S", no_argument, 0, SKIP_INSTALLABLE_PKGS_OPT},
        {"show", required_argument, 0, SHOW_OPT},
        {"w", required_argument, 0, SHOW_OPT},
        {"simulate", no_argument, 0, SIMULATE_OPT},
        {"t", no_argument, 0, SIMULATE_OPT},
        {"update", no_argument, 0, UPDATE_OPT},
        {"u", no_argument, 0, UPDATE_OPT},
        {"upgrade-all", no_argument, 0, UPGRADE_OPT},
        {"U", no_argument, 0, UPGRADE_OPT},
        {"yes", no_argument, 0, YES_OPT},
        {"version", no_argument, 0, VERSION_OPT},
        {0, 0, 0, 0}};

    /* initialization */
    setbuf(stdout, NULL);
#ifdef ENABLE_NLS
    setlocale(LC_ALL, "");
    textdomain(GETTEXT_PACKAGE);
#endif
#ifdef SLAPT_HAS_GPGME
    gpgme_check_version(NULL);
#ifdef ENABLE_NLS
    gpgme_set_locale(NULL, LC_CTYPE, setlocale(LC_CTYPE, NULL));
#endif
#endif
    curl_global_init(CURL_GLOBAL_ALL);
    uname(&uname_v);

    /* stop early */
    if (argc < 2) {
        help();
        exit(EXIT_SUCCESS);
    }

    int only_flags = 0;
    bool prompt = true, do_dep = true, simulate = false, skip_installable_pkgs = false;
    char *config_file = NULL, *postcmd = NULL;
    slapt_vector_t *names = slapt_vector_t_init(free);
    int c = -1, option_index = 0, action = 0;
    while ((c = getopt_long_only(argc, argv, "", long_options, &option_index)) != -1) {
        switch (c) {
        case HELP_OPT:
            help();
            exit(EXIT_SUCCESS);
        case VERSION_OPT:
            version();
            exit(EXIT_SUCCESS);
        case UPDATE_OPT:
            action = UPDATE_OPT;
            break;
        case UPGRADE_OPT:
            action = UPGRADE_OPT;
            break;
        case LIST_OPT:
            action = LIST_OPT;
            break;
        case CLEAN_OPT:
            action = CLEAN_OPT;
            break;
        case FETCH_OPT:
            action = FETCH_OPT;
            slapt_vector_t_add(names, strdup(optarg));
            break;
        case SEARCH_OPT:
            action = SEARCH_OPT;
            slapt_vector_t_add(names, strdup(optarg));
            break;
        case SHOW_OPT:
            action = SHOW_OPT;
            slapt_vector_t_add(names, strdup(optarg));
            break;
        case BUILD_OPT:
            action = BUILD_OPT;
            slapt_vector_t_add(names, strdup(optarg));
            break;
        case INSTALL_OPT:
            action = INSTALL_OPT;
            slapt_vector_t_add(names, strdup(optarg));
            break;
        case YES_OPT:
            prompt = false;
            break;
        case SIMULATE_OPT:
            simulate = true;
            break;
        case NODEP_OPT:
            do_dep = false;
            break;
        case CONFIG_OPT:
            config_file = strdup(optarg);
            break;
        case POSTCMD_OPT:
            postcmd = strdup(optarg);
            break;
        case BUILD_ONLY_OPT:
            only_flags |= BUILD_ONLY_FLAG;
            break;
        case FETCH_ONLY_OPT:
            only_flags |= FETCH_ONLY_FLAG;
            break;
        case SKIP_INSTALLABLE_PKGS_OPT:
            skip_installable_pkgs = true;
            break;
        default:
            help();
            exit(EXIT_FAILURE);
        }
    }

    if ((action == UPGRADE_OPT) && (optind < argc)) {
        fprintf(stderr, gettext("Individual packages not accepted when upgrading all slackbuilds\n"));
        exit(EXIT_FAILURE);
    }

    if ((only_flags & BUILD_ONLY_FLAG) && (only_flags & FETCH_ONLY_FLAG)) {
        fprintf(stderr, gettext("build-only and fetch-only options are mutually exclusive\n"));
        exit(EXIT_FAILURE);
    }

    /* add extra arguments */
    while (optind < argc) {
        slapt_vector_t_add(names, strdup(argv[optind]));
        ++optind;
    }

    slapt_src_config *config = NULL;
    if (config_file != NULL)
        config = slapt_src_read_config(config_file);
    else
        config = slapt_src_read_config(SLAPT_SRC_RC);
    if (config == NULL) {
        fprintf(stderr, gettext("Failed to read %s\n"), SLAPT_SRC_RC);
        exit(EXIT_FAILURE);
    }

    /* honor command line option */
    config->do_dep = do_dep;
    config->prompt = prompt;
    config->postcmd = postcmd; /* to be freed in slapt_src_config_free */

    init_builddir(config);
    if ((chdir(config->builddir)) != 0) {
        perror(gettext("Failed to chdir to build directory"));
        exit(EXIT_FAILURE);
    }

    slapt_vector_t *sbs = NULL;
    slapt_vector_t *remote_sbs = NULL;
    slapt_vector_t *installed = NULL;
    slapt_vector_t *available = NULL;

    /* setup, fetch, and other preparation steps */
    switch (action) {
    case CLEAN_OPT:
    case UPDATE_OPT:
        break; /* nothing to do here */
    case LIST_OPT:
    case SEARCH_OPT:
    case SHOW_OPT:
        remote_sbs = slapt_src_get_available_slackbuilds();
        break;
    case FETCH_OPT:
    case BUILD_OPT:
    case INSTALL_OPT:
    case UPGRADE_OPT:
        remote_sbs = slapt_src_get_available_slackbuilds();
        installed = slapt_get_installed_pkgs();

        if (skip_installable_pkgs) {
            slapt_config_t *slapt_config = slapt_config_t_read(RC_DIR "/slapt-getrc");
            if ((chdir(slapt_config->working_dir)) == 0) {
                available = slapt_get_available_pkgs();
                if ((chdir(config->builddir)) != 0) {
                    perror(gettext("Failed to chdir to build directory"));
                    exit(EXIT_FAILURE);
                }
            } else {
                skip_installable_pkgs = false;
            }
            slapt_config_t_free(slapt_config);
        }

        /* convert all names to slackbuilds */
        if (names->size > 0) {
            sbs = slapt_src_names_to_slackbuilds(config, remote_sbs, names, installed);
            if (sbs == NULL || sbs->size == 0) {
                printf(gettext("Unable to find all specified slackbuilds.\n"));
                exit(EXIT_FAILURE);
            }
        } else if (action == UPGRADE_OPT) {
            /* for each entry in 'installed' see if it's available as a slackbuild */
            slapt_vector_t_foreach(const slapt_pkg_t *, pkg, installed) {
                slapt_vector_t *matches = slapt_vector_t_search(remote_sbs, sb_compare_pkg_to_name, pkg->name);
                if (!matches) {
                    continue;
                }
                slapt_vector_t_foreach(const slapt_src_slackbuild *, upgrade_sb, matches) {
                    if (slapt_pkg_t_cmp_versions(upgrade_sb->version, pkg->version) == 1) {
                        // optionally skip packages that come from slapt-get
                        if (skip_installable_pkgs) {
                            if (slapt_get_newest_pkg(available, pkg->name)) {
                                continue;
                            }
                        }
                        slapt_vector_t_add(names, strdup(upgrade_sb->name));
                    }
                }
                slapt_vector_t_free(matches);
            }

            sbs = slapt_src_names_to_slackbuilds(config, remote_sbs, names, installed);
        }
        /* provide summary */
        if (!simulate)
            action = show_summary(sbs, names, action, prompt);
        break;
    default:
        help();
        exit(EXIT_FAILURE);
    }

    /*
  ** When upgrade-all is specified the sbs list is automatically
  ** determined. Transform the action flag accord to any additional
  ** flags.
  */
    if (action == UPGRADE_OPT) {
        if (only_flags & BUILD_ONLY_FLAG) {
            action = BUILD_OPT;
        } else if (only_flags & FETCH_ONLY_FLAG) {
            action = FETCH_OPT;
        } else {
            action = INSTALL_OPT;
        }
    }

    /* now, actually do what was requested */
    switch (action) {
    case UPDATE_OPT:
        if (!slapt_src_update_slackbuild_cache(config))
            exit(EXIT_FAILURE);
        break;

    case FETCH_OPT:
        ;
        bool old_prompt = config->prompt;
        config->prompt = false;
        slapt_vector_t_foreach(slapt_src_slackbuild *, fetch_sb, sbs) {
            if (simulate) {
                printf(gettext("FETCH: %s\n"), fetch_sb->name);
                continue;
            } else
                slapt_src_fetch_slackbuild(config, fetch_sb);
        }
        config->prompt = old_prompt;
        break;

    case BUILD_OPT:
        ;
        slapt_vector_t_foreach(slapt_src_slackbuild *, build_sb, sbs) {
            const size_t nv_len = strlen(build_sb->name) + strlen(build_sb->version) + 2;
            char namever[nv_len];
            const int r = snprintf(namever, nv_len, "%s:%s", build_sb->name, build_sb->version);
            if (r <= 0 || (size_t)r + 1 != nv_len) {
                exit(EXIT_FAILURE);
            }

            if (simulate) {
                printf(gettext("BUILD: %s\n"), build_sb->name);
                continue;
            }

            if (!slapt_src_fetch_slackbuild(config, build_sb))
                exit(EXIT_FAILURE);
            slapt_src_build_slackbuild(config, build_sb);

            /* XXX we assume if we didn't request the slackbuild, then it is a dependency, and needs to be installed */
            slapt_vector_t *name_matches = slapt_vector_t_search(names, sb_compare_name_to_name, build_sb->name);
            slapt_vector_t *namever_matches = slapt_vector_t_search(names, sb_compare_name_to_name, namever);
            if (name_matches == NULL && namever_matches == NULL) {
                slapt_src_install_slackbuild(config, build_sb);
            }

            if (name_matches)
                slapt_vector_t_free(name_matches);
            if (namever_matches)
                slapt_vector_t_free(namever_matches);
        }
        break;

    case INSTALL_OPT:
        ;
        slapt_vector_t_foreach(slapt_src_slackbuild *, install_sb, sbs) {
            if (simulate) {
                printf(gettext("INSTALL: %s\n"), install_sb->name);
                continue;
            }

            if (!slapt_src_fetch_slackbuild(config, install_sb))
                exit(EXIT_FAILURE);
            slapt_src_build_slackbuild(config, install_sb);
            slapt_src_install_slackbuild(config, install_sb);
        }
        break;

    case SEARCH_OPT: {
        ;
        slapt_vector_t *search = slapt_src_search_slackbuild_cache(remote_sbs, names);
        slapt_vector_t_foreach(slapt_src_slackbuild *, search_sb, search) {
            printf("%s:%s - %s\n",
                   search_sb->name,
                   search_sb->version,
                   search_sb->short_desc != NULL ? search_sb->short_desc : "");
        }
        slapt_vector_t_free(search);
    } break;

    case LIST_OPT:
        ;
        slapt_vector_t_foreach(slapt_src_slackbuild *, list_sb, remote_sbs) {
            printf("%s:%s - %s\n",
                   list_sb->name,
                   list_sb->version,
                   list_sb->short_desc != NULL ? list_sb->short_desc : "");
        }
        break;

    case SHOW_OPT: {
        ;
        slapt_vector_t_foreach(const char *, show_name, names) {
            slapt_vector_t *parts = slapt_parse_delimited_list(show_name, ':');
            const char *name = parts->items[0];
            const char *ver = NULL;

            if (parts->size > 1)
                ver = parts->items[1];

            const slapt_src_slackbuild *sb = slapt_src_get_slackbuild(remote_sbs, name, ver);

            if (sb != NULL) {
                printf(gettext("SlackBuild Name: %s\n"), sb->name);
                printf(gettext("SlackBuild Version: %s\n"), sb->version);
                printf(gettext("SlackBuild Category: %s\n"), sb->location);
                printf(gettext("SlackBuild Description: %s\n"), sb->short_desc != NULL ? sb->short_desc : "");

                printf(gettext("SlackBuild Files:\n"));

                slapt_vector_t_foreach(char *, f, sb->files) {
                    printf(" %s\n", f);
                }

                if (sb->requires != NULL)
                    printf(gettext("SlackBuild Requires: %s\n"), sb->requires);

                 printf("\n");

                /* slapt_src_slackbuild_free (sb); NO FREE */
            }
            slapt_vector_t_free(parts);
        }
    } break;
    case CLEAN_OPT:
        clean(config);
        break;
    default:
        help();
        exit(EXIT_FAILURE);
    }

    if (names != NULL)
        slapt_vector_t_free(names);
    if (sbs != NULL)
        slapt_vector_t_free(sbs);
    if (remote_sbs != NULL)
        slapt_vector_t_free(remote_sbs);
    if (installed != NULL)
        slapt_vector_t_free(installed);
    if (available != NULL)
        slapt_vector_t_free(available);
    if (config_file != NULL)
        free(config_file);

    slapt_src_config_free(config);

    return 0;
}

static int show_summary(slapt_vector_t *sbs, slapt_vector_t *names, int action, bool prompt)
{
    size_t line_len = 0;

    if (!sbs || sbs->size == 0) {
        printf(gettext("Done\n"));
        return action;
    }

    printf(gettext("The following packages will be %s:\n"),
           action == INSTALL_OPT ? gettext("installed")
                                 : action == BUILD_OPT ? gettext("built")
                                                       : gettext("fetched"));

    slapt_vector_t_foreach(const char *, sb_name, names) {
        slapt_vector_t *parts = slapt_parse_delimited_list(sb_name, ':');
        const char *name = parts->items[0];
        const size_t name_len = strlen(name);

        if (line_len < 80 - name_len) {
            printf("%s ", name);
            line_len += name_len + 1;
        } else {
            printf("\n %s ", name);
            line_len = name_len + 2;
        }

        slapt_vector_t_free(parts);
    }
    printf("\n");

    if (names->size < sbs->size) {
        line_len = 0;

        printf(gettext("The following dependent slackbuilds will be built and installed:\n"));

        slapt_vector_t_foreach(const slapt_src_slackbuild *, sb, sbs) {
            const char *name = sb->name;
            const char *ver = sb->version;
            char *namever = slapt_malloc(sizeof *namever * (strlen(name) + strlen(ver) + 2));
            sprintf(namever, "%s:%s", name, ver);

            slapt_vector_t *name_matches = slapt_vector_t_search(names, sb_compare_name_to_name, (char *)name);
            slapt_vector_t *namever_matches = slapt_vector_t_search(names, sb_compare_name_to_name, namever);
            if (name_matches == NULL && namever_matches == NULL) {
                const size_t name_len = strlen(name);

                if (line_len < 80 - name_len) {
                    printf("%s ", name);
                    line_len += name_len + 1;
                } else {
                    printf("\n %s ", name);
                    line_len = name_len + 2;
                }
            }
            if (name_matches)
                slapt_vector_t_free(name_matches);
            if (namever_matches)
                slapt_vector_t_free(namever_matches);
            free(namever);
        }
        printf("\n");
    }

    if ((sbs->size > 0) && (prompt == true)) {
        if (slapt_ask_yes_no(gettext("Do you want to continue? [y/N] ")) != 1) {
            printf(gettext("Abort.\n"));
            exit(EXIT_SUCCESS);
        }
    }

    return action;
}

static void clean(slapt_src_config *config)
{
    struct dirent *file = NULL;
    DIR *builddir = opendir(config->builddir);
    if (builddir != NULL) {
        while ((file = readdir(builddir)) != NULL) {
            struct stat stat_buf;

            if (strcmp(file->d_name, "..") == 0 || strcmp(file->d_name, ".") == 0)
                continue;

            if (lstat(file->d_name, &stat_buf) == -1)
                continue;

            if (S_ISDIR(stat_buf.st_mode)) {
                const size_t command_len = strlen(file->d_name) + 8;
                char command[command_len];
                const int r = snprintf(command, command_len, "rm -rf %s", file->d_name);
                if (r <= 0 || (size_t)r + 1 == command_len) {
                    int sys_r = system(command);
                    if (sys_r != 0)
                        exit(EXIT_FAILURE);
                }
            }
        }
        closedir(builddir);
    }
}
