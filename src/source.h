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
slapt_src_slackbuild_list *slapt_src_names_to_slackbuilds (slapt_src_slackbuild_list *, slapt_list_t *, slapt_pkg_list_t *);
slapt_src_slackbuild_list *slapt_src_get_slackbuilds_from_file (const char *);
void slapt_src_write_slackbuilds_to_file (slapt_src_slackbuild_list *, const char *);
slapt_src_slackbuild_list *slapt_src_search_slackbuild_cache (slapt_src_slackbuild_list *, slapt_list_t *);
slapt_src_slackbuild *slapt_src_get_slackbuild (slapt_src_slackbuild_list *, const char *);

#endif
