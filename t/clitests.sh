#!/bin/bash
set -eoxu

TEST_TMPDIR=$(mktemp -d)
trap 'echo FAILED at line ${LINENO} >&2;rm -rf ${TEST_TMPDIR};' err
trap 'rm -rf ${TEST_TMPDIR};' exit

slaptsrc="${1}"

config=${TEST_TMPDIR}/config
cat > "${config}" << EOF
SOURCE=http://www.slackbuilds.org/slackbuilds/15.0/
BUILDDIR=${TEST_TMPDIR}/slapt-src
PKGEXT=txz
EOF
cat "${config}"

${slaptsrc} --config "${config}" --help
${slaptsrc} --config "${config}" --version
${slaptsrc} --config "${config}" --update
${slaptsrc} --config "${config}" --list
${slaptsrc} --config "${config}" --search test
${slaptsrc} --config "${config}" --show dos2unix
${slaptsrc} --config "${config}" --upgrade-all -t

${slaptsrc} --config "${config}" --show dos2unix
${slaptsrc} --config "${config}" --show "$(${slaptsrc} --config "${config}" --search dos2unix | head -n1)"
${slaptsrc} --config "${config}" --show nosuchanythingshouldeverbehere
${slaptsrc} --config "${config}" --show :
${slaptsrc} --config "${config}" --fetch dos2unix -t
${slaptsrc} --config "${config}" --build dos2unix -t
${slaptsrc} --config "${config}" --install dos2unix -t
${slaptsrc} --config "${config}" --fetch dos2unix -y
${slaptsrc} --config "${config}" --clean


OFFLINE_TMPDIR=${TEST_TMPDIR}/offline
OFFLINE_REPO=${OFFLINE_TMPDIR}/repo
OFFLINE_BUILDDIR=${OFFLINE_TMPDIR}/build
OFFLINE_CONFIG=${OFFLINE_TMPDIR}/config
OFFLINE_SOURCE="file://${OFFLINE_REPO}/"

mkdir -p "${OFFLINE_REPO}/foo"
cat > "${OFFLINE_REPO}/foo/foo.SlackBuild" << 'EOF'
#!/bin/sh
echo right > foo-1.0-x86_64-me.txz
echo wrong > someone-1.0-x86_64-1.txz
EOF

mkdir -p "${OFFLINE_REPO}/fail"
cat > "${OFFLINE_REPO}/fail/fail.SlackBuild" << 'EOF'
#!/bin/sh
exit 1
EOF

mkdir -p "${OFFLINE_REPO}/readme"
cat > "${OFFLINE_REPO}/readme/readme.SlackBuild" << 'EOF'
#!/bin/sh
echo right > readme-1.0-x86_64-me.txz
EOF

# note: the trailing spaces on empty "SLACKBUILD <FIELD>: " lines are
# intentional
cat > "${OFFLINE_REPO}/SLACKBUILDS.TXT" << EOF
SLACKBUILD NAME: alpha
SLACKBUILD SOURCEURL: ${OFFLINE_SOURCE}
SLACKBUILD LOCATION: alpha/
SLACKBUILD FILES: alpha.SlackBuild
SLACKBUILD VERSION: 1.0
SLACKBUILD DOWNLOAD: 
SLACKBUILD DOWNLOAD_x86_64: 
SLACKBUILD MD5SUM: 
SLACKBUILD MD5SUM_x86_64: 
SLACKBUILD REQUIRES: gamma
SLACKBUILD SHORT DESCRIPTION: dep cycle member

SLACKBUILD NAME: beta
SLACKBUILD SOURCEURL: ${OFFLINE_SOURCE}
SLACKBUILD LOCATION: beta/
SLACKBUILD FILES: beta.SlackBuild
SLACKBUILD VERSION: 1.0
SLACKBUILD SHORT DESCRIPTION: adjacent block
SLACKBUILD NAME: gamma
SLACKBUILD SOURCEURL: ${OFFLINE_SOURCE}
SLACKBUILD LOCATION: gamma/
SLACKBUILD FILES: gamma.SlackBuild
SLACKBUILD VERSION: 1.0
SLACKBUILD DOWNLOAD: 
SLACKBUILD DOWNLOAD_x86_64: 
SLACKBUILD MD5SUM: 
SLACKBUILD MD5SUM_x86_64: 
SLACKBUILD REQUIRES: delta
SLACKBUILD SHORT DESCRIPTION: adjacent block, cycle member

SLACKBUILD NAME: delta
SLACKBUILD SOURCEURL: ${OFFLINE_SOURCE}
SLACKBUILD LOCATION: delta/
SLACKBUILD FILES: delta.SlackBuild
SLACKBUILD VERSION: 1.0
SLACKBUILD DOWNLOAD: 
SLACKBUILD DOWNLOAD_x86_64: 
SLACKBUILD MD5SUM: 
SLACKBUILD MD5SUM_x86_64: 
SLACKBUILD REQUIRES: gamma
SLACKBUILD SHORT DESCRIPTION: cycle member

SLACKBUILD NAME: diam
SLACKBUILD SOURCEURL: ${OFFLINE_SOURCE}
SLACKBUILD LOCATION: diam/
SLACKBUILD FILES: diam.SlackBuild
SLACKBUILD VERSION: 1.0
SLACKBUILD REQUIRES: d1 d2
SLACKBUILD SHORT DESCRIPTION: diamond top

SLACKBUILD NAME: d1
SLACKBUILD SOURCEURL: ${OFFLINE_SOURCE}
SLACKBUILD LOCATION: d1/
SLACKBUILD FILES: d1.SlackBuild
SLACKBUILD VERSION: 1.0
SLACKBUILD REQUIRES: d3
SLACKBUILD SHORT DESCRIPTION: diamond

SLACKBUILD NAME: d2
SLACKBUILD SOURCEURL: ${OFFLINE_SOURCE}
SLACKBUILD LOCATION: d2/
SLACKBUILD FILES: d2.SlackBuild
SLACKBUILD VERSION: 1.0
SLACKBUILD REQUIRES: d3
SLACKBUILD SHORT DESCRIPTION: diamond

SLACKBUILD NAME: d3
SLACKBUILD SOURCEURL: ${OFFLINE_SOURCE}
SLACKBUILD LOCATION: d3/
SLACKBUILD FILES: d3.SlackBuild
SLACKBUILD VERSION: 1.0
SLACKBUILD SHORT DESCRIPTION: diamond bottom

SLACKBUILD NAME: foo
SLACKBUILD SOURCEURL: ${OFFLINE_SOURCE}
SLACKBUILD LOCATION: foo/
SLACKBUILD FILES: foo.SlackBuild
SLACKBUILD VERSION: 1.0
SLACKBUILD DOWNLOAD: 
SLACKBUILD DOWNLOAD_x86_64: 
SLACKBUILD MD5SUM: 
SLACKBUILD MD5SUM_x86_64: 
SLACKBUILD REQUIRES: 
SLACKBUILD SHORT DESCRIPTION: tag fixture

SLACKBUILD NAME: fail
SLACKBUILD SOURCEURL: ${OFFLINE_SOURCE}
SLACKBUILD LOCATION: fail/
SLACKBUILD FILES: fail.SlackBuild
SLACKBUILD VERSION: 1.0
SLACKBUILD SHORT DESCRIPTION: failing build for the error-message test

SLACKBUILD NAME: evil4;\$(pwned)
SLACKBUILD SOURCEURL: ${OFFLINE_SOURCE}
SLACKBUILD LOCATION: evil/
SLACKBUILD FILES: evil.SlackBuild
SLACKBUILD VERSION: 1.0
SLACKBUILD SHORT DESCRIPTION: must be dropped

SLACKBUILD NAME: nover
SLACKBUILD SOURCEURL: ${OFFLINE_SOURCE}
SLACKBUILD LOCATION: nover/
SLACKBUILD FILES: nover.SlackBuild
SLACKBUILD VERSION: 
SLACKBUILD SHORT DESCRIPTION: must be dropped

SLACKBUILD NAME: absloc
SLACKBUILD SOURCEURL: ${OFFLINE_SOURCE}
SLACKBUILD LOCATION: /tmp/x/out
SLACKBUILD FILES: absloc.SlackBuild
SLACKBUILD VERSION: 1.0
SLACKBUILD SHORT DESCRIPTION: absolute location, must be dropped

SLACKBUILD NAME: travloc
SLACKBUILD SOURCEURL: ${OFFLINE_SOURCE}
SLACKBUILD LOCATION: ../out
SLACKBUILD FILES: travloc.SlackBuild
SLACKBUILD VERSION: 1.0
SLACKBUILD SHORT DESCRIPTION: traversal location, must be dropped

SLACKBUILD NAME: travfile
SLACKBUILD SOURCEURL: ${OFFLINE_SOURCE}
SLACKBUILD LOCATION: travfile/
SLACKBUILD FILES: ../../filepwn
SLACKBUILD VERSION: 1.0
SLACKBUILD SHORT DESCRIPTION: traversal file, must be dropped

SLACKBUILD NAME: noloc
SLACKBUILD SOURCEURL: ${OFFLINE_SOURCE}
SLACKBUILD FILES: noloc.SlackBuild
SLACKBUILD VERSION: 1.0
SLACKBUILD SHORT DESCRIPTION: no location, must be dropped

SLACKBUILD NAME: dotloc
SLACKBUILD SOURCEURL: ${OFFLINE_SOURCE}
SLACKBUILD LOCATION: ./system/dotloc
SLACKBUILD FILES: dotloc.SlackBuild
SLACKBUILD VERSION: 1.0
SLACKBUILD SHORT DESCRIPTION: dot-slash location, must survive

SLACKBUILD NAME: readme
SLACKBUILD SOURCEURL: ${OFFLINE_SOURCE}
SLACKBUILD LOCATION: readme/
SLACKBUILD FILES: readme.SlackBuild
SLACKBUILD VERSION: 1.0
SLACKBUILD DOWNLOAD: 
SLACKBUILD DOWNLOAD_x86_64: 
SLACKBUILD MD5SUM: 
SLACKBUILD MD5SUM_x86_64: 
SLACKBUILD REQUIRES: %README%
SLACKBUILD SHORT DESCRIPTION: readme fixture

SLACKBUILD NAME: .hidden
SLACKBUILD SOURCEURL: ${OFFLINE_SOURCE}
SLACKBUILD LOCATION: hidden/
SLACKBUILD FILES: hidden.SlackBuild
SLACKBUILD VERSION: 1.0
SLACKBUILD SHORT DESCRIPTION: leading-dot name, must be dropped

SLACKBUILD NAME: ..
SLACKBUILD SOURCEURL: ${OFFLINE_SOURCE}
SLACKBUILD LOCATION: dots/
SLACKBUILD FILES: dots.SlackBuild
SLACKBUILD VERSION: 1.0
SLACKBUILD SHORT DESCRIPTION: dot-dot name, must be dropped

SLACKBUILD NAME: 
SLACKBUILD SOURCEURL: ${OFFLINE_SOURCE}
SLACKBUILD LOCATION: emptyname/
SLACKBUILD FILES: emptyname.SlackBuild
SLACKBUILD VERSION: 1.0
SLACKBUILD SHORT DESCRIPTION: unnamed entry must be dropped

EOF

cat > "${OFFLINE_CONFIG}" << EOF
SOURCE=${OFFLINE_SOURCE}
BUILDDIR=${OFFLINE_BUILDDIR}
PKGEXT=txz
PKGTAG=me
EOF

if ${slaptsrc} --config "${OFFLINE_TMPDIR}/missing" --list 2> "${OFFLINE_TMPDIR}/cfg.err"; then
    echo "a missing config file must fail" >&2
    exit 1
fi
grep -qF "${OFFLINE_TMPDIR}/missing" "${OFFLINE_TMPDIR}/cfg.err"

${slaptsrc} --config "${OFFLINE_CONFIG}" --update

NO_RC_DIR=${OFFLINE_TMPDIR}/no-rc
mkdir -p "${NO_RC_DIR}" "${OFFLINE_TMPDIR}/var/log/packages"
if ! ROOT=${OFFLINE_TMPDIR} SLAPT_SRC_RC_DIR="${NO_RC_DIR}" ${slaptsrc} --config "${OFFLINE_CONFIG}" --upgrade-all -S -t 2> "${OFFLINE_TMPDIR}/norc.err"; then
    echo "-S without slapt-getrc must not fail" >&2
    exit 1
fi
grep -q "Failed to open ${NO_RC_DIR}/slapt-getrc" "${OFFLINE_TMPDIR}/norc.err"

OFFLINE_LIST=$(${slaptsrc} --config "${OFFLINE_CONFIG}" --list)
for name in alpha beta gamma delta diam d1 d2 d3 foo dotloc; do
    printf '%s\n' "${OFFLINE_LIST}" | grep -q "${name}:"
done

CACHE_NAMES=$(grep '^SLACKBUILD NAME: ' "${OFFLINE_BUILDDIR}/slackbuilds_data" | sed 's/SLACKBUILD NAME: //')
if [ "$(printf '%s\n' "${CACHE_NAMES}" | LC_ALL=C sort)" != "${CACHE_NAMES}" ]; then
    echo "the cache file must be written in name-sorted order" >&2
    exit 1
fi
if printf '%s\n' "${OFFLINE_LIST}" | grep -q evil; then
    echo "metacharacter name must be dropped from the cache" >&2
    exit 1
fi
if printf '%s\n' "${OFFLINE_LIST}" | grep -q 'unnamed entry must be dropped'; then
    echo "an entry without a name must be dropped from the cache" >&2
    exit 1
fi
if printf '%s\n' "${OFFLINE_LIST}" | grep -q nover; then
    echo "version-less entry must be dropped from the cache" >&2
    exit 1
fi

for bad in absloc travloc travfile noloc .hidden ..; do
    if printf '%s\n' "${OFFLINE_LIST}" | grep -qF "${bad}:"; then
        echo "${bad}: invalid entry must be dropped from the cache" >&2
        exit 1
    fi
done

NOEOL_REPO=${OFFLINE_TMPDIR}/noeol-repo
NOEOL_BUILDDIR=${OFFLINE_TMPDIR}/noeol-builddir
NOEOL_CONFIG=${OFFLINE_TMPDIR}/noeol-config
mkdir -p "${NOEOL_REPO}"
printf '%s\n' \
    "SLACKBUILD NAME: first" \
    "SLACKBUILD LOCATION: first/" \
    "SLACKBUILD FILES: first.SlackBuild" \
    "SLACKBUILD VERSION: 1.0" \
    "SLACKBUILD SHORT DESCRIPTION: first entry" \
    "" \
    "SLACKBUILD NAME: lasty" \
    "SLACKBUILD LOCATION: lasty/" \
    "SLACKBUILD FILES: lasty.SlackBuild" \
    "SLACKBUILD VERSION: 1.0" \
    "SLACKBUILD SHORT DESCRIPTION: final entry, no trailing blank line" > "${NOEOL_REPO}/SLACKBUILDS.TXT"
cat > "${NOEOL_CONFIG}" << EOF
SOURCE=file://${NOEOL_REPO}/
BUILDDIR=${NOEOL_BUILDDIR}
PKGEXT=txz
EOF
${slaptsrc} --config "${NOEOL_CONFIG}" --update
NOEOL_LIST=$(${slaptsrc} --config "${NOEOL_CONFIG}" --list)
if ! printf '%s\n' "${NOEOL_LIST}" | grep -q '^lasty:'; then
    echo "a list without a trailing blank line must keep its final entry" >&2
    exit 1
fi

if ${slaptsrc} --config "${OFFLINE_CONFIG}" --build alpha -t 2> "${OFFLINE_TMPDIR}/cycle.err"; then
    echo "dependency cycle must be rejected" >&2
    exit 1
fi
grep -q 'Circular dependency' "${OFFLINE_TMPDIR}/cycle.err"

${slaptsrc} --config "${OFFLINE_CONFIG}" --build diam -t

if ${slaptsrc} --config "${OFFLINE_CONFIG}" --build foo nosuch -t 2> "${OFFLINE_TMPDIR}/unknown.err"; then
    echo "unknown name in a multi-name request must be rejected" >&2
    exit 1
fi
grep -q 'Unknown slackbuild: nosuch' "${OFFLINE_TMPDIR}/unknown.err"

cp "${OFFLINE_REPO}/SLACKBUILDS.TXT" "${OFFLINE_TMPDIR}/list.bak"
: > "${OFFLINE_REPO}/SLACKBUILDS.TXT"
rm -f "${OFFLINE_BUILDDIR}"/.file:*
if ${slaptsrc} --config "${OFFLINE_CONFIG}" --update 2> "${OFFLINE_TMPDIR}/empty.err"; then
    echo "an empty list must not overwrite the cache" >&2
    exit 1
fi
grep -q 'Refusing to overwrite' "${OFFLINE_TMPDIR}/empty.err"
cp "${OFFLINE_TMPDIR}/list.bak" "${OFFLINE_REPO}/SLACKBUILDS.TXT"
printf '%s\n' "$(${slaptsrc} --config "${OFFLINE_CONFIG}" --list)" | grep -q 'foo:'

gzip -c "${OFFLINE_REPO}/SLACKBUILDS.TXT" > "${OFFLINE_REPO}/SLACKBUILDS.TXT.gz"
rm -f "${OFFLINE_BUILDDIR}"/.file:*
${slaptsrc} --config "${OFFLINE_CONFIG}" --update
printf 'not gzip' > "${OFFLINE_BUILDDIR}"/.file:*SLACKBUILDS.TXT.gz
if ${slaptsrc} --config "${OFFLINE_CONFIG}" --update > "${OFFLINE_TMPDIR}/gz.out" 2>&1; then
    echo "a corrupt gzip list file must be a hard error" >&2
    exit 1
fi
grep -q 'empty or corrupt gzip' "${OFFLINE_TMPDIR}/gz.out"
rm -f "${OFFLINE_REPO}/SLACKBUILDS.TXT.gz" "${OFFLINE_BUILDDIR}"/.file:*
${slaptsrc} --config "${OFFLINE_CONFIG}" --update
printf '%s\n' "$(${slaptsrc} --config "${OFFLINE_CONFIG}" --list)" | grep -q 'foo:'

cat > "${OFFLINE_TMPDIR}/wsconfig" << EOF
# a comment
   # an indented comment
SOURCE=${OFFLINE_SOURCE}
BUILDDIR=${OFFLINE_BUILDDIR}
PKGEXT=txz
PKGTAG= me
EOF
WSPOSTPROCESS_SCRIPT="${OFFLINE_TMPDIR}/wspost.sh"
cat > "${WSPOSTPROCESS_SCRIPT}" << EOF
#!/bin/sh
echo "picked:\$1" > ${OFFLINE_TMPDIR}/wspicked.txt
EOF
chmod +x "${WSPOSTPROCESS_SCRIPT}"
${slaptsrc} --config "${OFFLINE_TMPDIR}/wsconfig" --build foo -y --postprocess "${WSPOSTPROCESS_SCRIPT}"
grep -q 'picked:foo-1.0-x86_64-me.txz' "${OFFLINE_TMPDIR}/wspicked.txt"

if ${slaptsrc} --config "${OFFLINE_CONFIG}" --build fail -y > "${OFFLINE_TMPDIR}/failbuild.out" 2>&1; then
    echo "a failing .SlackBuild must fail the build" >&2
    exit 1
fi
grep -q 'sh fail.SlackBuild Failed' "${OFFLINE_TMPDIR}/failbuild.out"

LONGCOMP_A=$(printf 'x%.0s' $(seq 100))
LONGCOMP_B=$(printf 'y%.0s' $(seq 100))
LONGCOMP_C=$(printf 'z%.0s' $(seq 100))
mkdir -p "${OFFLINE_TMPDIR}/${LONGCOMP_A}/${LONGCOMP_B}"
cat > "${OFFLINE_TMPDIR}/longconfig" << EOF
SOURCE=${OFFLINE_SOURCE}
BUILDDIR=${OFFLINE_TMPDIR}/${LONGCOMP_A}/${LONGCOMP_B}/${LONGCOMP_C}
EOF
if ${slaptsrc} --config "${OFFLINE_TMPDIR}/longconfig" --update 2> "${OFFLINE_TMPDIR}/long.err"; then
    echo "overlong BUILDDIR must be rejected" >&2
    exit 1
fi
grep -q 'BUILDDIR too long' "${OFFLINE_TMPDIR}/long.err"

mkdir -p "${OFFLINE_BUILDDIR}/-n/nested" "${OFFLINE_BUILDDIR}/normal" "${OFFLINE_TMPDIR}/outside"
BADDIR='foo;$(pwned)'
mkdir -p "${OFFLINE_BUILDDIR}/${BADDIR}/sub"
touch "${OFFLINE_BUILDDIR}/-n/nested/f" "${OFFLINE_BUILDDIR}/${BADDIR}/sub/f" "${OFFLINE_BUILDDIR}/normal/f"
touch "${OFFLINE_TMPDIR}/outside/target"
ln -s "${OFFLINE_TMPDIR}/outside/target" "${OFFLINE_BUILDDIR}/normal/link"
touch "${OFFLINE_BUILDDIR}/topfile"

${slaptsrc} --config "${OFFLINE_CONFIG}" --clean

if [ -d "${OFFLINE_BUILDDIR}/-n" ] || [ -d "${OFFLINE_BUILDDIR}/${BADDIR}" ] || [ -d "${OFFLINE_BUILDDIR}/normal" ]; then
    echo "clean must remove all build dirs" >&2
    exit 1
fi
[ -e "${OFFLINE_TMPDIR}/outside/target" ]
[ -f "${OFFLINE_BUILDDIR}/topfile" ]

POSTPROCESS_SCRIPT="${OFFLINE_TMPDIR}/post.sh"
cat > "${POSTPROCESS_SCRIPT}" << EOF
#!/bin/sh
echo "picked:\$1" > ${OFFLINE_TMPDIR}/picked.txt
EOF
chmod +x "${POSTPROCESS_SCRIPT}"

${slaptsrc} --config "${OFFLINE_CONFIG}" --build foo -y --postprocess "${POSTPROCESS_SCRIPT}"
grep -q 'picked:foo-1.0-x86_64-me.txz' "${OFFLINE_TMPDIR}/picked.txt"

if ${slaptsrc} --config "${OFFLINE_CONFIG}" --list --clean 2> "${OFFLINE_TMPDIR}/multi.err"; then
    echo "multiple actions must fail" >&2
    exit 1
fi
grep -q 'Specify only one action' "${OFFLINE_TMPDIR}/multi.err"

if ${slaptsrc} --config "${OFFLINE_CONFIG}" --build readme -y > "${OFFLINE_TMPDIR}/readme.out" 2>&1; then
    echo "missing README must fail" >&2
    exit 1
fi
grep -q 'Failed to open the README for' "${OFFLINE_TMPDIR}/readme.out"


# Configuration parsing checks

printf 'SOURCE=%s\r\nBUILDDIR=%s/crlfbuild\r\nPKGEXT=txz\r\nPKGTAG=me\r\n' "${OFFLINE_SOURCE}" "${OFFLINE_TMPDIR}" > "${OFFLINE_TMPDIR}/crlfconfig"
if ! ${slaptsrc} --config "${OFFLINE_TMPDIR}/crlfconfig" --update > "${OFFLINE_TMPDIR}/crlf.out" 2>&1; then
    echo "CRLF config must update like an LF config" >&2
    exit 1
fi
if grep -q 'bad/illegal format\|No BUILDDIR specified' "${OFFLINE_TMPDIR}/crlf.out"; then
    echo "a CR must not leak into a config value" >&2
    exit 1
fi
[ -d "${OFFLINE_TMPDIR}/crlfbuild" ]

printf 'SOURCE=%s\r\n\r\nBUILDDIR=%s/crlfblankbuild\r\n' "${OFFLINE_SOURCE}" "${OFFLINE_TMPDIR}" > "${OFFLINE_TMPDIR}/crlfblankconfig"
if ! ${slaptsrc} --config "${OFFLINE_TMPDIR}/crlfblankconfig" --update > /dev/null 2>&1; then
    echo "a CRLF blank line must not break the config parse" >&2
    exit 1
fi
[ -d "${OFFLINE_TMPDIR}/crlfblankbuild" ]

printf 'SOURCE=%s\nBUILDDIR=%s/noeolbuild' "${OFFLINE_SOURCE}" "${OFFLINE_TMPDIR}" > "${OFFLINE_TMPDIR}/noeolconfig"
if ! ${slaptsrc} --config "${OFFLINE_TMPDIR}/noeolconfig" --update > /dev/null 2>&1; then
    echo "a config without a trailing newline must still be read" >&2
    exit 1
fi
[ -d "${OFFLINE_TMPDIR}/noeolbuild" ]
if [ -d "${OFFLINE_TMPDIR}/noeolbuil" ]; then
    echo "the final character must not be chopped off the last value" >&2
    exit 1
fi

printf 'SOURCE=%s\nBUILDDIR=%s/trailbuild   \n' "${OFFLINE_SOURCE}" "${OFFLINE_TMPDIR}" > "${OFFLINE_TMPDIR}/trailconfig"
if ! ${slaptsrc} --config "${OFFLINE_TMPDIR}/trailconfig" --update > /dev/null 2>&1; then
    echo "trailing whitespace must not break the config parse" >&2
    exit 1
fi
[ -d "${OFFLINE_TMPDIR}/trailbuild" ]
if [ -d "${OFFLINE_TMPDIR}/trailbuild   " ]; then
    echo "trailing spaces must not be part of BUILDDIR" >&2
    exit 1
fi

printf 'SOURCE=%s\rBUILDDIR=%s/cronlybuild\r' "${OFFLINE_SOURCE}" "${OFFLINE_TMPDIR}" > "${OFFLINE_TMPDIR}/cronlyconfig"
if ${slaptsrc} --config "${OFFLINE_TMPDIR}/cronlyconfig" --update 2> "${OFFLINE_TMPDIR}/cronly.err"; then
    echo "a CR-only config must be rejected, not silently half-read" >&2
    exit 1
fi
grep -q 'No BUILDDIR specified' "${OFFLINE_TMPDIR}/cronly.err"
if [ -d "${OFFLINE_TMPDIR}/cronlybuild" ]; then
    echo "a CR-only config must not create a build directory" >&2
    exit 1
fi

mkdir -p "${OFFLINE_TMPDIR}/crlfrepo/x"
printf 'SLACKBUILD NAME: crlfalpha\r\nSLACKBUILD SOURCEURL: file://%s/crlfrepo/\r\nSLACKBUILD LOCATION: x/\r\nSLACKBUILD FILES: crlfalpha.SlackBuild\r\nSLACKBUILD VERSION: 1.0\r\nSLACKBUILD SHORT DESCRIPTION: crlf repo fixture\r\n\r\n' "${OFFLINE_TMPDIR}" > "${OFFLINE_TMPDIR}/crlfrepo/SLACKBUILDS.TXT"
printf 'SOURCE=file://%s/crlfrepo/\nBUILDDIR=%s/crlfrepobuild\nPKGEXT=txz\nPKGTAG=me\n' "${OFFLINE_TMPDIR}" "${OFFLINE_TMPDIR}" > "${OFFLINE_TMPDIR}/crlfrepoc"
if ! ${slaptsrc} --config "${OFFLINE_TMPDIR}/crlfrepoc" --update > /dev/null 2>&1; then
    echo "a CRLF SLACKBUILDS.TXT must still parse" >&2
    exit 1
fi
CRLF_LIST=$(${slaptsrc} --config "${OFFLINE_TMPDIR}/crlfrepoc" --list)
if printf '%s\n' "${CRLF_LIST}" | grep -q $'\r'; then
    echo "a CR must not reach the parsed SLACKBUILDS.TXT fields" >&2
    exit 1
fi
printf '%s\n' "${CRLF_LIST}" | grep -q 'crlfalpha:'

find "${TEST_TMPDIR}"
