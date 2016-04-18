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

#include <slapt.h>
#include <sys/utsname.h>
#ifndef __SLAPT_SRC_SOURCE_H__
#define __SLAPT_SRC_SOURCE_H__

#define SLAPT_SRC_RC "/etc/slapt-get/slapt-srcrc"
#define SLAPT_SRC_DATA_FILE "slackbuilds_data"
#define SLAPT_SRC_SOURCE_TOKEN "SOURCE="
#define SLAPT_SRC_BUILDDIR_TOKEN "BUILDDIR="
#define SLAPT_SRC_PKGEXT_TOKEN "PKGEXT="
#define SLAPT_SRC_PKGTAG_TOKEN "PKGTAG="
#define SLAPT_SRC_SOURCES_LIST_GZ "SLACKBUILDS.TXT.gz"
#define SLAPT_SRC_SOURCES_LIST "SLACKBUILDS.TXT"

typedef struct _slapt_src_config_
{
  slapt_list_t *sources;
  char *builddir;
  char *pkgext;
  char *pkgtag;
  char *postcmd;
  SLAPT_BOOL_T do_dep;
} slapt_src_config;
slapt_src_config *slapt_src_config_init (void);
void slapt_src_config_free (slapt_src_config *config);
slapt_src_config *slapt_src_read_config (const char *filename);

typedef struct _slapt_src_slackbuild_
{
  char *name;
  char *version;
  char *location;
  char *sb_source_url;
  slapt_list_t *files;
  char *download;
  char *download_x86_64;
  char *md5sum;
  char *md5sum_x86_64;
  char *short_desc;
  char *requires;
} slapt_src_slackbuild;
slapt_src_slackbuild *slapt_src_slackbuild_init (void);
void slapt_src_slackbuild_free (slapt_src_slackbuild *);

typedef struct _slapt_src_slackbuilds_
{
  slapt_src_slackbuild **slackbuilds;
  unsigned int count;
  SLAPT_BOOL_T free_slackbuilds;
} slapt_src_slackbuild_list;
slapt_src_slackbuild_list *slapt_src_slackbuild_list_init (void);
void slapt_src_slackbuild_list_free (slapt_src_slackbuild_list *);
void slapt_src_slackbuild_list_add (slapt_src_slackbuild_list *, slapt_src_slackbuild *);

int slapt_src_update_slackbuild_cache (slapt_src_config *);
slapt_src_slackbuild_list *slapt_src_get_available_slackbuilds (void);
int slapt_src_fetch_slackbuild (slapt_src_config *, slapt_src_slackbuild *);
int slapt_src_build_slackbuild (slapt_src_config *, slapt_src_slackbuild *);
int slapt_src_install_slackbuild (slapt_src_config *, slapt_src_slackbuild *);
slapt_src_slackbuild_list *slapt_src_names_to_slackbuilds (slapt_src_config *, slapt_src_slackbuild_list *, slapt_list_t *, slapt_pkg_list_t *);
slapt_src_slackbuild_list *slapt_src_get_slackbuilds_from_file (const char *);
void slapt_src_write_slackbuilds_to_file (slapt_src_slackbuild_list *, const char *);
slapt_src_slackbuild_list *slapt_src_search_slackbuild_cache (slapt_src_slackbuild_list *, slapt_list_t *);
slapt_src_slackbuild *slapt_src_get_slackbuild (slapt_src_slackbuild_list *, const char *, const char *);

#endif
