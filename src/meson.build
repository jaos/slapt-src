sources = [
  'main.c',
  'source.c',
  'source.h',
]

configure_file(output: 'config.h', configuration: configuration)
slapt_src = executable('slapt-src', sources, dependencies : deps, install: true, install_dir: get_option('bindir'))
