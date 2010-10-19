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
static char *gen_short_pkg_description (slapt_src_slackbuild *);

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
  printf (gettext ("Usage: %s [action]\n"), PACKAGE);
  printf ("  --update |-u  - %s\n", gettext ("update local cache of remote slackbuilds"));
  printf ("  --list   |-l  - %s\n", gettext ("list available slackbuilds"));
  printf (gettext ("Usage: %s [option(s)] [action] [slackbuild(s)]\n"), PACKAGE);
  printf ("  --search      |-s  - %s\n", gettext ("search available slackbuilds"));
  printf ("  --show        |-w  - %s\n", gettext ("show specified slackbuilds"));
  printf ("  --install     |-i  - %s\n", gettext ("fetch, build, and install the specified slackbuild(s)"));
  printf ("  --build       |-b  - %s\n", gettext ("only fetch and build the specified slackbuild(s)"));
  printf ("  --fetch       |-f  - %s\n", gettext ("only fetch the specified slackbuild(s)"));
  printf ("  --yes         |-y  - %s\n", gettext ("do not prompt"));
  printf ("  --config      |-c  - %s\n", gettext ("use the specified configuration file"));
  printf ("  --no-dep      |-n  - %s\n", gettext ("do not look for dependencies"));
  printf ("  --postprocess |-p  - %s\n", gettext ("run specified command on generated package"));
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
  int c = -1, option_index = 0, action = 0, i = 0;
  slapt_list_t *names = slapt_init_list ();
  slapt_src_config *config = NULL;
  slapt_src_slackbuild_list *sbs = NULL;
  slapt_src_slackbuild_list *remote_sbs = NULL;
  slapt_pkg_list_t *installed = NULL;
  SLAPT_BOOL_T prompt = SLAPT_TRUE, do_dep = SLAPT_TRUE;
  char *config_file = NULL, *postcmd = NULL;

  static struct option long_options[] = {
    {"version",     no_argument,        0, VERSION_OPT},
    {"help",        no_argument,        0, HELP_OPT},
    {"update",      no_argument,        0, UPDATE_OPT},
    {"list",        no_argument,        0, LIST_OPT},
    {"search",      required_argument,  0, SEARCH_OPT},
    {"s",           required_argument,  0, SEARCH_OPT},
    {"show",        required_argument,  0, SHOW_OPT},
    {"w",           required_argument,  0, SHOW_OPT},
    {"fetch",       required_argument,  0, FETCH_OPT},
    {"build",       required_argument,  0, BUILD_OPT},
    {"install",     required_argument,  0, INSTALL_OPT},
    {"yes",         no_argument,        0, YES_OPT},
    {"no-dep",      no_argument,        0, NODEP_OPT},
    {"config",      required_argument,  0, CONFIG_OPT},
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
      case FETCH_OPT: action = FETCH_OPT; slapt_add_list_item (names, optarg); break;
      case SEARCH_OPT: action = SEARCH_OPT; slapt_add_list_item (names, optarg); break;
      case SHOW_OPT: action = SHOW_OPT; slapt_add_list_item (names, optarg); break;
      case BUILD_OPT: action = BUILD_OPT; slapt_add_list_item (names, optarg); break;
      case INSTALL_OPT: action = INSTALL_OPT; slapt_add_list_item (names, optarg); break;
      case YES_OPT: prompt = SLAPT_FALSE; break;
      case NODEP_OPT: do_dep = SLAPT_FALSE; break;
      case CONFIG_OPT: config_file = strdup (optarg); break;
      case POSTCMD_OPT: postcmd = strdup (optarg); break;
      default: help (); exit (EXIT_SUCCESS);
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
  config->postcmd = postcmd;

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
      action = show_summary (sbs, names, action, prompt);
    break;
  }


  /* now, actually do what was requested */
  switch (action) {

    case UPDATE_OPT:
      slapt_src_update_slackbuild_cache (config);
    break;

    case FETCH_OPT:
      for (i = 0; i < sbs->count; i++) {
        slapt_src_fetch_slackbuild (config, sbs->slackbuilds[i]);
      }
    break;

    case BUILD_OPT:
      for (i = 0; i < sbs->count; i++) {
        slapt_src_slackbuild *sb = sbs->slackbuilds[i];

        slapt_src_fetch_slackbuild (config, sb);
        slapt_src_build_slackbuild (config, sb);

        /* XXX we assume if we didn't request the slackbuild, then
           it is a dependency, and needs to be installed */
        if (slapt_search_list (names, sb->name) == NULL)
          slapt_src_install_slackbuild (config, sbs->slackbuilds[i]);
      }
    break;

    case INSTALL_OPT:
      for (i = 0; i < sbs->count; i++) {
        slapt_src_slackbuild *sb = sbs->slackbuilds[i];

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
          printf ("%s-%s: %s\n",
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
        printf ("%s-%s: %s\n",
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
          slapt_src_slackbuild *sb = slapt_src_get_slackbuild (remote_sbs, names->items[i]);

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
        }
      }
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

  return 0;
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
    const char *name = names->items[i];
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

  }
  printf ("\n");

  if (names->count < sbs->count) {
    line_len = 0;

    printf (gettext ("The following dependent slackbuilds will be built and installed:\n"));

    for (i = 0; i < sbs->count; i++) {
      const char *name = sbs->slackbuilds[i]->name;

      if (slapt_search_list (names, name) == NULL) {
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

