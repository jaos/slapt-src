/*
 * Copyright (C) 2010-2016 Jason Woodward <woodwardj at jaos dot org>
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

/* getopt */
extern char *optarg;
extern int optind, opterr, optopt;

static int show_summary (slapt_src_slackbuild_list *, slapt_list_t *, int, SLAPT_BOOL_T);
static void clean (slapt_src_config *config);

void version (void)
{
  printf (gettext ("%s version %s\n"), PACKAGE, VERSION);
  printf ("Jason Woodward <woodwardj at jaos dot org>\n");
  printf ("\n");
  printf ("This program is free software; you can redistribute it and/or modify\n");
  printf ("it under the terms of the GNU General Public License as published by\n");
  printf ("the Free Software Foundation; either version 2 of the License, or\n");
  printf ("any later version.\n");
  printf ("This program is distributed in the hope that it will be useful,\n");
  printf ("\n");
  printf ("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
  printf ("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
  printf ("GNU Library General Public License for more details.\n");
  printf ("\n");
  printf ("You should have received a copy of the GNU General Public License\n");
  printf ("along with this program; if not, write to the Free Software\n");
  printf ("Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n");
}

void help (void)
{
  printf (gettext ("%s - A SlackBuild utility\n"), PACKAGE);
  printf (gettext ("Usage: %s [option(s)] [action] [slackbuild(s)]\n"), PACKAGE);
  printf ("  -u, --update           %s\n", gettext ("update local cache of remote slackbuilds"));
  printf ("  -l, --list             %s\n", gettext ("list available slackbuilds"));
  printf ("  -e, --clean            %s\n", gettext ("clean build directory"));
  printf ("  -s, --search           %s\n", gettext ("search available slackbuilds"));
  printf ("  -w, --show             %s\n", gettext ("show specified slackbuilds"));
  printf ("  -i, --install          %s\n", gettext ("fetch, build, and install the specified slackbuild(s)"));
  printf ("  -b, --build            %s\n", gettext ("only fetch and build the specified slackbuild(s)"));
  printf ("  -f, --fetch            %s\n", gettext ("only fetch the specified slackbuild(s)"));
  printf ("  -v, --version\n");
  printf ("  -h, --help\n");
  printf (" %s:\n", gettext ("Options") );
  printf ("  -y, --yes              %s\n", gettext ("do not prompt"));
  printf ("  -t, --simulate         %s\n", gettext ("show what will be done"));
  printf ("  -c, --config=FILE      %s\n", gettext ("use the specified configuration file"));
  printf ("  -n, --no-dep           %s\n", gettext ("do not look for dependencies"));
  printf ("  -p, --postprocess=CMD  %s\n", gettext ("run specified command on generated package"));
}

#define VERSION_OPT 'v'
#define HELP_OPT 'h'
#define UPDATE_OPT 'u'
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

struct utsname uname_v; /* for .machine */

static void init_builddir (slapt_src_config *config)
{
  DIR *builddir = NULL;
  mode_t mode = S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

  if ( (builddir = opendir (config->builddir)) == NULL ) {
    if ( mkdir (config->builddir, mode) == -1 ) {
      printf (gettext ("Failed to create build directory [%s]\n"), config->builddir);

      if (errno)
        perror (config->builddir);

      exit (EXIT_FAILURE);
    }
  } else {
    closedir (builddir);
  }
}

int main (int argc, char *argv[])
{
  int c = -1, option_index = 0, action = 0, i = 0, rval = 0;
  slapt_list_t *names = slapt_init_list ();
  slapt_src_config *config = NULL;
  slapt_src_slackbuild_list *sbs = NULL;
  slapt_src_slackbuild_list *remote_sbs = NULL;
  slapt_pkg_list_t *installed = NULL;
  SLAPT_BOOL_T prompt = SLAPT_TRUE, do_dep = SLAPT_TRUE, simulate = SLAPT_FALSE;
  char *config_file = NULL, *postcmd = NULL;

  static struct option long_options[] = {
    {"version",     no_argument,        0, VERSION_OPT},
    {"help",        no_argument,        0, HELP_OPT},
    {"update",      no_argument,        0, UPDATE_OPT},
    {"list",        no_argument,        0, LIST_OPT},
    {"clean",       no_argument,        0, CLEAN_OPT},
    {"e",           no_argument,        0, CLEAN_OPT},
    {"search",      required_argument,  0, SEARCH_OPT},
    {"s",           required_argument,  0, SEARCH_OPT},
    {"show",        required_argument,  0, SHOW_OPT},
    {"w",           required_argument,  0, SHOW_OPT},
    {"fetch",       required_argument,  0, FETCH_OPT},
    {"build",       required_argument,  0, BUILD_OPT},
    {"install",     required_argument,  0, INSTALL_OPT},
    {"yes",         no_argument,        0, YES_OPT},
    {"simulate",    no_argument,        0, SIMULATE_OPT},
    {"t",           no_argument,        0, SIMULATE_OPT},
    {"no-dep",      no_argument,        0, NODEP_OPT},
    {"config",      required_argument,  0, CONFIG_OPT},
    {"c",           required_argument,  0, CONFIG_OPT},
    {"postprocess", required_argument,  0, POSTCMD_OPT},
    {0, 0, 0, 0}
  };

  /* initialization */
  setbuf (stdout, NULL);
  #ifdef ENABLE_NLS
  setlocale (LC_ALL,"");
  textdomain (GETTEXT_PACKAGE);
  #endif
  #ifdef SLAPT_HAS_GPGME
  gpgme_check_version (NULL);
  #ifdef ENABLE_NLS
  gpgme_set_locale (NULL, LC_CTYPE, setlocale (LC_CTYPE, NULL));
  #endif
  #endif
  curl_global_init (CURL_GLOBAL_ALL);
  uname (&uname_v);

  /* stop early */
  if (argc < 2) {
    help ();
    exit (EXIT_SUCCESS);
  }

  while ( (c = getopt_long_only (argc, argv, "", long_options, &option_index)) != -1) {
    switch (c) {
      case HELP_OPT: help (); break;
      case VERSION_OPT: version (); break;
      case UPDATE_OPT: action = UPDATE_OPT; break;
      case LIST_OPT: action = LIST_OPT; break;
      case CLEAN_OPT: action = CLEAN_OPT; break;
      case FETCH_OPT: action = FETCH_OPT; slapt_add_list_item (names, optarg); break;
      case SEARCH_OPT: action = SEARCH_OPT; slapt_add_list_item (names, optarg); break;
      case SHOW_OPT: action = SHOW_OPT; slapt_add_list_item (names, optarg); break;
      case BUILD_OPT: action = BUILD_OPT; slapt_add_list_item (names, optarg); break;
      case INSTALL_OPT: action = INSTALL_OPT; slapt_add_list_item (names, optarg); break;
      case YES_OPT: prompt = SLAPT_FALSE; break;
      case SIMULATE_OPT: simulate = SLAPT_TRUE; break;
      case NODEP_OPT: do_dep = SLAPT_FALSE; break;
      case CONFIG_OPT: config_file = strdup (optarg); break;
      case POSTCMD_OPT: postcmd = strdup (optarg); break;
      default: help (); exit (EXIT_FAILURE);
    }
  }

  /* add extra arguments */
  while (optind < argc) {
    slapt_add_list_item (names, argv[optind]);
    ++optind;
  }

  if (config_file != NULL)
    config = slapt_src_read_config (config_file);
  else
    config = slapt_src_read_config (SLAPT_SRC_RC);
  if (config == NULL) {
    fprintf (stderr, gettext ("Failed to read %s\n"), SLAPT_SRC_RC);
    exit (EXIT_FAILURE);
  }

  /* honor command line option */
  config->do_dep = do_dep;
  config->postcmd = postcmd; /* to be freed in slapt_src_config_free */

  init_builddir (config);
  if ( (chdir (config->builddir)) != 0) {
    perror (gettext ("Failed to chdir to build directory"));
    exit (EXIT_FAILURE);
  }

  /* setup, fetch, and other preperation steps */
  switch (action) {
    case LIST_OPT:
    case SEARCH_OPT:
    case SHOW_OPT:
      remote_sbs = slapt_src_get_available_slackbuilds ();
    break;
    case FETCH_OPT:
    case BUILD_OPT:
    case INSTALL_OPT:
      remote_sbs = slapt_src_get_available_slackbuilds ();
      installed = slapt_get_installed_pkgs ();
      /* convert all names to slackbuilds */
      if (names->count > 0) {
        sbs = slapt_src_names_to_slackbuilds (config, remote_sbs, names, installed);
        if (sbs == NULL || sbs->count == 0) {
          printf (gettext ("Unable to find all specified slackbuilds.\n"));
          exit (EXIT_FAILURE);
        }
      }
      /* provide summary */
      if (!simulate)
        action = show_summary (sbs, names, action, prompt);
    break;
  }


  /* now, actually do what was requested */
  switch (action) {

    case UPDATE_OPT:
      rval = slapt_src_update_slackbuild_cache (config);
    break;

    case FETCH_OPT:
      for (i = 0; i < sbs->count; i++) {
        if (simulate) {
          printf (gettext ("FETCH: %s\n"), sbs->slackbuilds[i]->name);
          continue;
        } else
          slapt_src_fetch_slackbuild (config, sbs->slackbuilds[i]);
      }
    break;

    case BUILD_OPT:
      for (i = 0; i < sbs->count; i++) {
        slapt_src_slackbuild *sb = sbs->slackbuilds[i];
        int r = 0, nv_len = strlen (sb->name) + strlen (sb->version) + 2;
        char *namever = slapt_malloc (sizeof *namever * nv_len);
        r = snprintf (namever, nv_len, "%s:%s", sb->name, sb->version);
        
        if (r+1 != nv_len)
           exit (EXIT_FAILURE);

        if (simulate) {
          printf (gettext ("BUILD: %s\n"), sb->name);
          free (namever);
          continue;
        }

        slapt_src_fetch_slackbuild (config, sb);
        slapt_src_build_slackbuild (config, sb);

        /* XXX we assume if we didn't request the slackbuild, then
           it is a dependency, and needs to be installed */
        if (slapt_search_list (names, sb->name) == NULL && slapt_search_list (names, namever) == NULL)
          slapt_src_install_slackbuild (config, sbs->slackbuilds[i]);

        free (namever);
      }
    break;

    case INSTALL_OPT:
      for (i = 0; i < sbs->count; i++) {
        slapt_src_slackbuild *sb = sbs->slackbuilds[i];

        if (simulate) {
          printf (gettext ("INSTALL: %s\n"), sb->name);
          continue;
        }

        slapt_src_fetch_slackbuild (config, sb);
        slapt_src_build_slackbuild (config, sb);
        slapt_src_install_slackbuild (config, sb);
      }
    break;

    case SEARCH_OPT:
      {
        slapt_src_slackbuild_list *search = slapt_src_search_slackbuild_cache (remote_sbs, names);
        for (i = 0; i < search->count; i++) {
          slapt_src_slackbuild *sb = search->slackbuilds[i];
          printf ("%s:%s - %s\n",
            sb->name,
            sb->version,
            sb->short_desc != NULL ? sb->short_desc : ""
          );
        }
        slapt_src_slackbuild_list_free (search);
      }
    break;

    case LIST_OPT:
      for (i = 0; i < remote_sbs->count; i++) {
        slapt_src_slackbuild *sb = remote_sbs->slackbuilds[i];
        printf ("%s:%s - %s\n",
          sb->name,
          sb->version,
          sb->short_desc != NULL ? sb->short_desc : ""
        );
      }
    break;

    case SHOW_OPT:
      {
        int c;
        for (i = 0; i < names->count; i++) {
          slapt_src_slackbuild *sb = NULL;
          slapt_list_t *parts      = slapt_parse_delimited_list (names->items[i], ':');
          const char *name         = parts->items[0];
          const char *ver          = NULL;

          if (parts->count > 1)
            ver = parts->items[1];

          sb = slapt_src_get_slackbuild (remote_sbs, name, ver);

          if (sb != NULL) {

            printf (gettext ("SlackBuild Name: %s\n"), sb->name);
            printf (gettext ("SlackBuild Version: %s\n"), sb->version);
            printf (gettext ("SlackBuild Category: %s\n"), sb->location);
            printf (gettext ("SlackBuild Description: %s\n"), sb->short_desc != NULL ? sb->short_desc : "");

            printf (gettext ("SlackBuild Files:\n"));

            for (c = 0; c < sb->files->count; c++) {
              printf (" %s\n", sb->files->items[c]);
            }

            if (sb->requires != NULL)
              printf (gettext ("SlackBuild Requires: %s\n"), sb->requires);

            if (i+1 != names->count)
              printf ("\n");

            /* slapt_src_slackbuild_free (sb); NO FREE */
          }
          slapt_free_list (parts);
        }
      }
    break;
    case CLEAN_OPT:
      clean (config);
    break;
  }

  if (names != NULL)
    slapt_free_list (names);
  if (sbs != NULL)
    slapt_src_slackbuild_list_free (sbs);
  if (remote_sbs != NULL)
    slapt_src_slackbuild_list_free (remote_sbs);
  if (installed != NULL)
    slapt_free_pkg_list (installed);
  if (config_file != NULL)
    free (config_file);

  slapt_src_config_free (config);

  return rval;
}

static int show_summary (slapt_src_slackbuild_list *sbs, slapt_list_t *names, int action, SLAPT_BOOL_T prompt)
{
  int i, line_len = 0;

  printf (gettext ("The following packages will be %s:\n"),
    action == INSTALL_OPT ? gettext ("installed")
    : action == BUILD_OPT ? gettext ("built")
    : gettext ("fetched")
  );

  for (i = 0; i < names->count; i++) {
    slapt_list_t *parts = slapt_parse_delimited_list (names->items[i], ':');
    const char *name = parts->items[0];
    int name_len = strlen (name);

    if (line_len == 0) {
      printf (" %s ", name);
      line_len += name_len + 1;
    } else if (line_len < 80 - name_len) {
      printf ("%s ", name);
      line_len += name_len + 1;
    } else {
      printf ("\n %s ", name);
      line_len = name_len + 2;
    }

    slapt_free_list (parts);
  }
  printf ("\n");

  if (names->count < sbs->count) {
    line_len = 0;

    printf (gettext ("The following dependent slackbuilds will be built and installed:\n"));

    for (i = 0; i < sbs->count; i++) {
      const char *name = sbs->slackbuilds[i]->name;
      const char *version = sbs->slackbuilds[i]->version;
      char *namever = slapt_malloc (sizeof *namever * (strlen (name) + strlen (version) + 2));
      sprintf (namever, "%s:%s", name, version);

      if (slapt_search_list (names, name) == NULL && slapt_search_list (names, namever) == NULL) {
        int name_len = strlen (name);

        if (line_len == 0) {
          printf (" %s ", name);
          line_len += name_len + 2;
        } else if (line_len < 80 - name_len) {
          printf ("%s ", name);
          line_len += name_len + 1;
        } else {
          printf ("\n %s ", name);
          line_len = name_len + 2;
        }

      }

      free (namever);
    }
    printf ("\n");
  }

  if (prompt == SLAPT_TRUE) {
    if (slapt_ask_yes_no (gettext ("Do you want to continue? [y/N] ")) != 1) {
      printf (gettext ("Abort.\n"));
      return 0;
    }
  }

  return action;
}

static void clean (slapt_src_config *config)
{
  struct dirent *file = NULL;
  DIR *builddir = opendir (config->builddir);
  if ( builddir != NULL ) {
    while ((file = readdir (builddir)) != NULL) {
      struct stat stat_buf;

      if (strcmp (file->d_name, "..") == 0 || strcmp (file->d_name, ".") == 0)
      continue;

      if (lstat (file->d_name, &stat_buf) == -1)
          continue;

      if ( S_ISDIR (stat_buf.st_mode)) {
        int r = 0, command_len = strlen (file->d_name)+8;
        char *command = slapt_malloc (sizeof *command * command_len);
        r = snprintf (command, command_len, "rm -rf %s", file->d_name);
        if (r+1 == command_len) {
          int sys_r = system (command);
          if (sys_r != 0)
            exit (EXIT_FAILURE);
        }
      }
    }
  }
  closedir (builddir);
}
