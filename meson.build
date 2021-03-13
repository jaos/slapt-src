project('slapt-src', 'c',
  version: '0.3.6',
  license: 'GPLv2',
  meson_version: '>= 0.53',
  default_options: [
    'buildtype=debugoptimized',
    'sysconfdir=/etc',
    'b_lto=true',
    'warning_level=3',
  ],
)
package = meson.project_name()
version = meson.project_version()
i18n = import('i18n')

# paths
prefix = get_option('prefix')
localedir = join_paths(prefix, get_option('localedir'))
datadir = join_paths(prefix, get_option('datadir'))
docdatadir = join_paths(datadir, 'doc', package)

configuration = configuration_data()
configuration.set_quoted('PACKAGE', package)
configuration.set_quoted('GETTEXT_PACKAGE', package)
configuration.set_quoted('VERSION', version)
if get_option('fakeroot').enabled()
  configuration.set('HAS_FAKEROOT', 1)
endif
if get_option('slkbuild').enabled()
  configuration.set('HAS_SLKBUILD', 1)
endif

# compiler based checks
libcurl = dependency('libcurl')
zlib = dependency('zlib')
openssl = dependency('openssl')
libgpgme = dependency('gpgme', required: false)
cc = meson.get_compiler('c')
libm = cc.find_library('m')
# libslapt = dependency('libslapt', version: '>=0.11.3') # use this when we have a few pkg-config enabled releases out
libslapt = cc.find_library('slapt', has_headers: ['slapt.h'])
if libslapt.found()
  # if libgpgme.found() and cc.has_function('slapt_add_pkg_source_gpg_key', dependencies: [libslapt])`
  if libgpgme.found() and cc.has_function('slapt_add_pkg_source_gpg_key', dependencies: [libcurl, zlib, openssl, libgpgme, libslapt])
    configuration.set('SLAPT_HAS_GPGME', 1)
  endif
endif

if get_option('fakeroot').enabled()
  find_program('fakeroot')
endif
if get_option('slkbuild').enabled()
  find_program('slkbuild')
endif

deps = [libcurl, zlib, openssl, libm, libgpgme, libslapt]

subdir('po')
subdir('src')

install_man('slapt-src.8')
install_data('slapt-src.ru.8', install_dir: join_paths(get_option('mandir'), 'ru', 'man8'), rename: 'slapt-src.8')
install_data('slapt-srcrc', install_dir: join_paths(get_option('sysconfdir'), 'slapt-get'), rename: 'slapt-srcrc.new')
install_data(['ChangeLog', 'COPYING', 'README'], install_dir: docdatadir)
