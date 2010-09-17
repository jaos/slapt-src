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

static char *gen_short_pkg_description (slapt_src_slackbuild *);

void version (void)
{
  printf (gettext("%s version %s\n"), PACKAGE, VERSION);
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
  printf (gettext("%s - A SlackBuild utility\n"), PACKAGE);
  printf (gettext("Usage: %s [action]\n"), PACKAGE);
  printf ("  --update |-u  - %s\n", "update local cache of remote slackbuilds");
  printf ("  --list   |-l  - %s\n", "list available slackbuilds");
  printf (gettext("Usage: %s [action] [slackbuild(s)]\n"), PACKAGE);
  printf ("  --search |-s  - %s\n", gettext("search available slackbuilds"));
  printf ("  --show   |-w  - %s\n", gettext("show specified slackbuilds"));
  printf ("  --install|-i  - %s\n", gettext("fetch, build, and install the specified slackbuild(s)"));
  printf ("  --build  |-b  - %s\n", gettext("only fetch and build the specified slackbuild(s)"));
  printf ("  --fetch  |-f  - %s\n", gettext("only fetch the specified slackbuild(s)"));
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

struct utsname uname_v; /* for .machine */

static void init_builddir (slapt_src_config *config)
{
  DIR *builddir = NULL;
  mode_t mode = S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

  if ( (builddir = opendir (config->builddir)) == NULL ) {
    if ( mkdir (config->builddir, mode) == -1 ) {
      printf ("Failed to create build  directory [%s]\n", config->builddir);

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

  static struct option long_options[] = {
    {"version", no_argument,        0, VERSION_OPT},
    {"help",    no_argument,        0, HELP_OPT},
    {"update",  no_argument,        0, UPDATE_OPT},
    {"list",    no_argument,        0, LIST_OPT},
    {"search",  required_argument,  0, SEARCH_OPT},
    {"s",       required_argument,  0, SEARCH_OPT},
    {"show",    required_argument,  0, SHOW_OPT},
    {"w",       required_argument,  0, SHOW_OPT},
    {"fetch",   required_argument,  0, FETCH_OPT},
    {"build",   required_argument,  0, BUILD_OPT},
    {"install", required_argument,  0, INSTALL_OPT},
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
      default: help (); exit (EXIT_SUCCESS);
    }
  }

  /* add extra arguments */
  while (optind < argc) {
    slapt_add_list_item (names, argv[optind]);
    ++optind;
  }

  config = slapt_src_read_config (SLAPT_SRC_RC);
  if (config == NULL) {
    fprintf (stderr,"Failed to read %s\n", SLAPT_SRC_RC);
    exit (EXIT_FAILURE);
  }

  init_builddir (config);
  if ( (chdir (config->builddir)) != 0) {
    perror ("Failed to chdir to build directory");
    exit (EXIT_FAILURE);
  }

  switch (action) {
    case SEARCH_OPT:
      remote_sbs = slapt_src_get_available_slackbuilds ();
    break;
    case FETCH_OPT:
    case BUILD_OPT:
    case INSTALL_OPT:
    case LIST_OPT:
    case SHOW_OPT:
      remote_sbs = slapt_src_get_available_slackbuilds ();
      /* convert all names to slackbuilds */
      if (names->count > 0) {
        sbs = slapt_src_names_to_slackbuilds (config, remote_sbs, names);
        if (sbs == NULL || sbs->count == 0) {
          printf (gettext("Unable to find all specified slackbuilds.\n"));
          exit (EXIT_FAILURE);
        }
      }
    break;
  }


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
        slapt_src_fetch_slackbuild (config, sbs->slackbuilds[i]);
        slapt_src_build_slackbuild (config, sbs->slackbuilds[i]);
      }
    break;

    case INSTALL_OPT:
      for (i = 0; i < sbs->count; i++) {
        slapt_src_fetch_slackbuild (config, sbs->slackbuilds[i]);
        slapt_src_build_slackbuild (config, sbs->slackbuilds[i]);
        slapt_src_install_slackbuild (config, sbs->slackbuilds[i]);
      }
    break;

    case SEARCH_OPT:
      {
        slapt_src_slackbuild_list *search = slapt_src_search_slackbuild_cache (remote_sbs, names);
        for (i = 0; i < search->count; i++) {
          char *short_desc = gen_short_pkg_description (search->slackbuilds[i]);
          printf ("%s - %s: %s\n",
            search->slackbuilds[i]->name,
            search->slackbuilds[i]->version,
            short_desc != NULL ? short_desc : ""
          );
          if (short_desc != NULL)
            free (short_desc);
        }
        slapt_src_slackbuild_list_free (search);
      }
    break;

    case LIST_OPT:
      for (i = 0; i < remote_sbs->count; i++) {
        char *short_desc = gen_short_pkg_description (remote_sbs->slackbuilds[i]);
        printf ("%s - %s: %s\n",
          remote_sbs->slackbuilds[i]->name,
          remote_sbs->slackbuilds[i]->version,
          short_desc != NULL ? short_desc : ""
        );
        if (short_desc != NULL)
          free (short_desc);
      }
    break;

    case SHOW_OPT:
      {
        int c;
        for (i = 0; i < names->count; i++) {
          slapt_src_slackbuild *sb = slapt_src_get_slackbuild (remote_sbs, names->items[i]);

          if (sb != NULL) {
            char *short_desc = gen_short_pkg_description (sb);
            char *desc = strdup(sb->readme);

            slapt_clean_description (desc, sb->name);

            printf (gettext("SlackBuild Name: %s\n"), sb->name);
            printf (gettext("SlackBuild Version: %s\n"), sb->version);
            printf (gettext("SlackBuild Category: %s\n"), sb->location);
            printf (gettext("SlackBuild Files:\n"));

            for (c = 0; c < sb->files->count; c++) {
              printf (" %s\n", sb->files->items[c]);
            }

            printf (gettext("SlackBuild README:\n%s\n"), desc);

            if (i+1 != names->count)
              printf ("\n");

            free (desc);
            free (short_desc);
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

  slapt_src_config_free (config);

  return 0;
}

static char *gen_short_pkg_description (slapt_src_slackbuild *sb)
{
  char *short_readme = NULL;
  size_t string_size = 0;

  if (sb->readme == NULL)
    return NULL;

  if (strchr (sb->readme,'\n') != NULL) {
    string_size = strlen (sb->readme) -
      (strlen (sb->name) + 2) - strlen (strchr (sb->readme,'\n'));
  }

  /* quit now if the readme is going to be empty */
  if ((int)string_size < 1)
    return NULL;

  short_readme = strndup ( sb->readme + (strlen (sb->name) + 2), string_size);

  return short_readme;
}
