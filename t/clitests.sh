#!/bin/bash
set -eoxu

TEST_TMPDIR=$(mktemp -d)
trap "rm -rf ${TEST_TMPDIR};" err exit

slaptsrc="${1}"
config=${TEST_TMPDIR}/config
cat > ${config} << EOF
SOURCE=http://www.slackbuilds.org/slackbuilds/14.2/
BUILDDIR=${TEST_TMPDIR}/slapt-src
PKGEXT=txz
EOF

${slaptsrc} --config "${config}" --help
${slaptsrc} --config "${config}" --version
${slaptsrc} --config "${config}" --update
${slaptsrc} --config "${config}" --list
${slaptsrc} --config "${config}" --search test
${slaptsrc} --config "${config}" --show z
${slaptsrc} --config "${config}" --upgrade-all -t
${slaptsrc} --config "${config}" --fetch z -t
${slaptsrc} --config "${config}" --build z -t
${slaptsrc} --config "${config}" --install z -t
${slaptsrc} --config "${config}" --fetch z -y
${slaptsrc} --config "${config}" --clean

find ${TEST_TMPDIR}
