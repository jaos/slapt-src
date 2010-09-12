#define _GNU_SOURCE
#include "source.h"

extern struct utsname u;

slapt_src_config *slapt_src_config_init (void)
{
  slapt_src_config *config = slapt_malloc (sizeof *config);
  config->sources = slapt_init_list ();
  config->builddir = NULL;
  return config;
}

void slapt_src_config_free (slapt_src_config *config)
{
  slapt_free_list (config->sources);
  if (config->builddir != NULL)
    free (config->builddir);
  free (config);
}

slapt_src_config *slapt_src_read_config (const char *filename)
{
  FILE *rc = NULL;
  slapt_src_config *config = NULL;
  char *buffer = NULL;
  size_t gb_length = 0;
  ssize_t g_size;

  rc = slapt_open_file (filename, "r");
  if (rc == NULL) return NULL;

  config = slapt_src_config_init ();

  while ( (g_size = getline (&buffer, &gb_length, rc) ) != EOF ) {
    char *token_ptr = NULL;
    buffer[g_size - 1] = '\0';

    if (strchr (buffer, '#') != NULL)
      continue;

    if ( (token_ptr = strstr (buffer,SLAPT_SRC_SOURCE_TOKEN)) != NULL ) {
      if ( strlen (token_ptr) > strlen (SLAPT_SRC_SOURCE_TOKEN) )
        slapt_add_list_item (config->sources, token_ptr + strlen (SLAPT_SRC_SOURCE_TOKEN));
    } else if ( (token_ptr = strstr (buffer,SLAPT_SRC_BUILDDIR_TOKEN)) != NULL ) {
      if ( strlen (token_ptr) > strlen (SLAPT_SRC_BUILDDIR_TOKEN) )
      config->builddir = strdup (token_ptr + strlen (SLAPT_SRC_BUILDDIR_TOKEN));
    }

  }

  fclose (rc);
  if (buffer != NULL)
    free (buffer);

  return config;
}

slapt_src_slackbuild *slapt_src_slackbuild_init (void)
{
  slapt_src_slackbuild *sb = slapt_malloc (sizeof *sb);
  sb->name = NULL;
  sb->version = NULL;
  sb->location = NULL;
  sb->sb_source_url = NULL;
  sb->readme = NULL;
  sb->download = NULL;
  sb->download_x86_64 = NULL;
  sb->md5sum = NULL;
  sb->md5sum_x86_64 = NULL;
  sb->files = slapt_init_list ();

  return sb;
}

void slapt_src_slackbuild_free (slapt_src_slackbuild *sb)
{
  if (sb->name != NULL)
    free (sb->name);
  if (sb->version != NULL)
    free (sb->version);
  if (sb->location != NULL)
    free (sb->location);
  if (sb->sb_source_url != NULL)
    free (sb->sb_source_url);
  if (sb->download != NULL)
    free (sb->download);
  if (sb->download_x86_64 != NULL)
    free (sb->download_x86_64);
  if (sb->md5sum != NULL)
    free (sb->md5sum);
  if (sb->md5sum_x86_64 != NULL)
    free (sb->md5sum_x86_64);
  if (sb->readme != NULL)
    free (sb->readme);

  slapt_free_list (sb->files);

  free (sb);
}

slapt_src_slackbuild_list *slapt_src_slackbuild_list_init (void)
{
  slapt_src_slackbuild_list *sbs = slapt_malloc (sizeof *sbs);
  sbs->slackbuilds = slapt_malloc (sizeof *sbs->slackbuilds);
  sbs->count = 0;

  return sbs;
}

void slapt_src_slackbuild_list_free (slapt_src_slackbuild_list *sbs)
{
  int i;

  for (i = 0; i < sbs->count; i++) {
    slapt_src_slackbuild_free (sbs->slackbuilds[i]);
  }

  free (sbs->slackbuilds);
  free (sbs);
}

void slapt_src_slackbuild_list_add (slapt_src_slackbuild_list *sbs, slapt_src_slackbuild *sb)
{
  slapt_src_slackbuild **realloc_tmp = realloc (sbs->slackbuilds, sizeof *sbs->slackbuilds * (sbs->count + 1));
  if (realloc_tmp == NULL) {
    perror ("realloc failed");
    exit (EXIT_FAILURE);
  }

  sbs->slackbuilds = realloc_tmp;
  sbs->slackbuilds[sbs->count] = sb;
  ++sbs->count;
}

int slapt_src_update_slackbuild_cache (slapt_src_config *config)
{
  int i;
  slapt_rc_config *slapt_config = slapt_init_config ();
  slapt_src_slackbuild_list *slackbuilds = slapt_src_slackbuild_list_init ();

  for (i = 0; i < config->sources->count; i++) {
    const char *url = config->sources->items[i];
    FILE *f = NULL;
    const char *err = NULL;
    slapt_src_slackbuild_list *sbs = NULL;
    char *filename = NULL, *local_head = NULL, *head = NULL;

    printf ("Fetching slackbuild list from %s...", url);

    filename = slapt_gen_filename_from_url (url, SLAPT_SRC_SOURCES_LIST);
    local_head = slapt_read_head_cache (filename);
    head = slapt_head_mirror_data (url, SLAPT_SRC_SOURCES_LIST);

    /* is it cached ? */
    if (head != NULL && local_head != NULL && strcmp (head, local_head) == 0) {
      printf ("Cached\n");
      sbs = slapt_src_get_slackbuilds_from_file (filename);
    } else {

      if ((f = slapt_open_file (filename,"w+b")) == NULL)
        exit (EXIT_FAILURE);

      err = slapt_get_mirror_data_from_source (f, slapt_config, url, SLAPT_SRC_SOURCES_LIST);
      fclose (f);

      if (!err) {
        printf ("Done\n");
        sbs = slapt_src_get_slackbuilds_from_file (filename);

        slapt_write_head_cache (head, filename);

      } else {
        fprintf (stderr, "Download failed: %s\n", err);
        slapt_clear_head_cache (filename);
      }
    }

    if (sbs != NULL) {
      int c;
      for (c = 0; c < sbs->count; c++) {
        sbs->slackbuilds[c]->sb_source_url = strdup (url);
        slapt_src_slackbuild_list_add (slackbuilds, sbs->slackbuilds[c]);
      }
      sbs->count = 0;
      slapt_src_slackbuild_list_free (sbs);
    }

    free (filename);
    free (local_head);
    free (head);
  }

  slapt_src_write_slackbuilds_to_file (slackbuilds, SLAPT_SRC_DATA_FILE);
  slapt_free_rc_config (slapt_config);
  slapt_src_slackbuild_list_free (slackbuilds);
  return 0;
}

static int sb_cmp (const void *a, const void *b)
{
  slapt_src_slackbuild *sb1 = *(slapt_src_slackbuild * const *)a;
  slapt_src_slackbuild *sb2 = *(slapt_src_slackbuild * const *)b;

  return strcmp(sb1->name, sb2->name);
}

void slapt_src_write_slackbuilds_to_file (slapt_src_slackbuild_list *sbs, const char *datafile)
{
  int i;
  FILE *f = slapt_open_file (datafile, "w+b");

  qsort( sbs->slackbuilds, sbs->count, sizeof(sbs->slackbuilds[0]), sb_cmp );

  for (i = 0; i < sbs->count; i++) {
    int c;
    /* write out package data */
    fprintf (f, "SLACKBUILD NAME: %s\n", sbs->slackbuilds[i]->name);
    fprintf (f, "SLACKBUILD LOCATION: %s\n", sbs->slackbuilds[i]->location);
    fprintf (f, "SLACKBUILD FILES: ");
    for (c = 0; c < sbs->slackbuilds[i]->files->count; c++) {
      if (c == (sbs->slackbuilds[i]->files->count - 1))
        fprintf (f, "%s\n", sbs->slackbuilds[i]->files->items[c]);
      else
        fprintf (f, "%s ", sbs->slackbuilds[i]->files->items[c]);
    }
    fprintf (f, "SLACKBUILD INFO:\n");
    fprintf (f, "VERSION=\"%s\"\n", sbs->slackbuilds[i]->version);
    fprintf (f, "DOWNLOAD=\"%s\"\n", sbs->slackbuilds[i]->download ? sbs->slackbuilds[i]->download : "");
    fprintf (f, "DOWNLOAD_x86_64=\"%s\"\n", sbs->slackbuilds[i]->download_x86_64 ? sbs->slackbuilds[i]->download_x86_64 : "");
    fprintf (f, "MD5SUM=\"%s\"\n", sbs->slackbuilds[i]->md5sum ? sbs->slackbuilds[i]->md5sum : "");
    fprintf (f, "MD5SUM_x86_64=\"%s\"\n", sbs->slackbuilds[i]->md5sum_x86_64 ? sbs->slackbuilds[i]->md5sum_x86_64 : "");
    fprintf (f, "SLACKBUILD README:\n");
    if (sbs->slackbuilds[i]->readme != NULL)  
      fprintf (f, "%s\n", sbs->slackbuilds[i]->readme);
    fprintf (f, "\n");
  }

  fclose (f);
}

slapt_src_slackbuild_list *slapt_src_get_slackbuilds_from_file (const char *datafile)
{
  FILE *f = NULL;
  char *buffer = NULL;
  size_t gb_length = 0;
  ssize_t g_size;
  slapt_src_slackbuild_list *sbs = slapt_src_slackbuild_list_init ();
  slapt_src_slackbuild *sb = NULL;
  enum { SLAPT_SRC_NOT_PARSING, SLAPT_SRC_PARSING, SLAPT_SRC_PARSING_INFO, SLAPT_SRC_PARSING_README } parse_state;
  parse_state = SLAPT_SRC_NOT_PARSING;

  f = fopen (datafile, "r");
  if (f == NULL) {
    printf ("Failed to open %s for reading\n", datafile);
    return sbs;
  }

  while ( (g_size = getline (&buffer, &gb_length, f) ) != EOF ) {
    char *token = NULL;
    buffer[g_size - 1] = '\0';

    if (strstr (buffer, "SLACKBUILD NAME:") != NULL) {
      parse_state = SLAPT_SRC_PARSING;
      sb = slapt_src_slackbuild_init ();
    }
    if (strstr (buffer, "SLACKBUILD INFO:") != NULL)
      parse_state = SLAPT_SRC_PARSING_INFO;
    if (strstr (buffer, "SLACKBUILD README:") != NULL)
      parse_state = SLAPT_SRC_PARSING_README;

    if ((strcmp (buffer, "") == 0) && (parse_state == SLAPT_SRC_PARSING_README)) {
      slapt_src_slackbuild_list_add (sbs, sb);
      /* do not free, in use in sbs list
      slapt_src_slackbuild_free (sb);
      */
      sb = NULL;
      parse_state = SLAPT_SRC_NOT_PARSING;
    }

    switch (parse_state) {
      case SLAPT_SRC_PARSING:
        if ( (sscanf (buffer, "SLACKBUILD NAME: %as", &token)) == 1) {
          sb->name = strdup (token);
          free (token);
        }

        if ( (sscanf (buffer, "SLACKBUILD LOCATION: %as", &token)) == 1) {
          sb->location = strdup (token);
          free (token);
        }

        if ( (sscanf (buffer, "SLACKBUILD FILES: %a[^\n]", &token)) == 1) {
          int c;
          slapt_list_t *files = slapt_parse_delimited_list (token, ' ');
          for (c = 0; c < files->count; c++) {
            slapt_add_list_item (sb->files, files->items[c]);
          }
          slapt_free_list (files);
          free (token);
        }
      break;

      case SLAPT_SRC_PARSING_INFO:
        if ( (sscanf (buffer, "VERSION=\"%a[^\"]\"", &token)) == 1) {
          sb->version = strdup (token);
          free (token);
        }
        if ( (sscanf (buffer, "DOWNLOAD=\"%a[^\"]\"", &token)) == 1) {
          sb->download = strdup (token);
          free (token);
        }
        if ( (sscanf (buffer, "DOWNLOAD_x86_64=\"%a[^\"]\"", &token)) == 1) {
          sb->download_x86_64 = strdup (token);
          free (token);
        }
        if ( (sscanf (buffer, "MD5SUM=\"%a[^\"]\"", &token)) == 1) {
          sb->md5sum = strdup (token);
          free (token);
        }
        if ( (sscanf (buffer, "MD5SUM_x86_64=\"%a[^\"]\"", &token)) == 1) {
          sb->md5sum_x86_64 = strdup (token);
          free (token);
        }
      break;

      case SLAPT_SRC_PARSING_README:
        if (strncmp (buffer, sb->name, strlen(sb->name)) == 0) {
          if (sb->readme != NULL) {
            sb->readme = realloc (sb->readme, sizeof *sb->readme * (strlen (sb->readme) + strlen (buffer) + 2));
            sb->readme = strcat (sb->readme, "\n");
            sb->readme = strcat (sb->readme, buffer);
          } else {
            sb->readme = strdup (buffer);
          }
      }
      break;

      default:
      break;
    }


  }

  if (buffer != NULL)
    free (buffer);

  fclose (f);
  return sbs;
}

slapt_src_slackbuild_list *slapt_src_get_available_slackbuilds ()
{
  return slapt_src_get_slackbuilds_from_file (SLAPT_SRC_DATA_FILE);
}

int slapt_src_fetch_slackbuild (slapt_src_config *config, slapt_src_slackbuild *sb)
{
  /* create working directory within build dir */
  /* fetch source and extra files into working directory */
  printf ("fetching %s\n", sb->name);
  return 0;
}

int slapt_src_build_slackbuild (slapt_src_config *config, slapt_src_slackbuild *sb)
{
  /* change to working directory */
  /* make sure slackbuild file is +x */

  /*
    make sure we set the locations for the slackbuilds to honor
  setenv ("TMP", "", 1);
  setenv ("OUTPUT", "", 1);
  setenv ("PKGTYPE", "", 1);
  */

  /* run slackbuild */

  /* go back */
  if (chdir (config->builddir) != 0) {}

  printf ("building %s\n", sb->name);
  return 0;
}

int slapt_src_install_slackbuild (slapt_src_config *config, slapt_src_slackbuild *sb)
{
  /* change to working directory */
  /* install built package using SLAPT_UPGRADE_CMD */
  printf ("installing %s\n", sb->name);
  return 0;
}

slapt_src_slackbuild_list *slapt_src_names_to_slackbuilds (slapt_src_config *config, slapt_src_slackbuild_list *available, slapt_list_t *names)
{
  int i;
  slapt_src_slackbuild_list *sbs = slapt_src_slackbuild_list_init ();

  for (i = 0; i < names->count; i++) {
    int c;
    slapt_list_t *search_names = slapt_init_list ();

    slapt_add_list_item(search_names, names->items[i]);
    slapt_src_slackbuild_list *search = slapt_src_search_slackbuild_cache (available, search_names);
/* FIXME
==27390==  Address 0x8e279a8 is 0 bytes after a block of size 8 alloc'd
==27390==    at 0x4C285A2: realloc (vg_replace_malloc.c:525)
==27390==    by 0x4022C5: slapt_src_slackbuild_list_add (source.c:125)
==27390==    by 0x402423: slapt_src_search_slackbuild_cache (source.c:458)
==27390==    by 0x4024C1: slapt_src_names_to_slackbuilds (source.c:405)
==27390==    by 0x401BAC: main (main.c:147)
*/

    /* TODO dep check here */
    for (c = 0; c < search->count; c++) {
      slapt_src_slackbuild_list_add (sbs, search->slackbuilds[i]);
/* FIXME
==27390== Invalid read of size 8
==27390==    at 0x4024DE: slapt_src_names_to_slackbuilds (source.c:417)
==27390==    by 0x401BAC: main (main.c:147)
*/
    }

    search->count = 0;
    slapt_src_slackbuild_list_free(search);
    slapt_free_list(search_names);
  }

  return sbs;
}

slapt_src_slackbuild_list *slapt_src_search_slackbuild_cache (slapt_src_slackbuild_list *remote_sbs, slapt_list_t *names)
{
  int i, n;
  slapt_src_slackbuild_list *sbs = slapt_src_slackbuild_list_init ();

  for (n = 0; n < names->count; n++) {
    slapt_regex_t *search_regex = slapt_init_regex (names->items[n]);
    if (search_regex == NULL)
      continue;

    for (i = 0; i < remote_sbs->count; i++) {
      int name_r = -1, version_r = -1, readme_r = -1;

      slapt_execute_regex (search_regex, remote_sbs->slackbuilds[i]->name);
      name_r    = search_regex->reg_return;

      slapt_execute_regex (search_regex, remote_sbs->slackbuilds[i]->location);
      version_r = search_regex->reg_return;

      if (remote_sbs->slackbuilds[i]->readme != NULL) {
        slapt_execute_regex (search_regex, remote_sbs->slackbuilds[i]->readme);
        readme_r  = search_regex->reg_return;
      }

      if (name_r == 0 || version_r == 0 || readme_r == 0)
        slapt_src_slackbuild_list_add (sbs, remote_sbs->slackbuilds[i]);
        

    }

    slapt_free_regex (search_regex);
  }

  return sbs;
}
