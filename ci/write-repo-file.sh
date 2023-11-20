#!/usr/bin/env bash
set -e

GITREF=`git rev-parse --short HEAD`

if [[ -z ${BRANCH} ]]; then
  BRANCH=`git name-rev $GITREF --name-only`
fi

if [[ $BRANCH =~ ^(tags/)?(v)[.0-9]+(-(rc)?([0-9]+))?$ ]]; then
  BUILD=
elif [[ ! -z ${DMC_REPO_BRANCH} ]]; then
  BUILD="${DMC_REPO_BRANCH}"
else
  BUILD="develop"
fi

DIST=$(rpm --eval "%{dist}" | cut -d. -f2)
DISTNAME=${DIST}

if [[ -z ${BUILD} ]]; then
	REPO_PATH="${DISTNAME}/\$basearch"
	BUILD="production"
elif [[ ${BUILD} == "develop" ]]; then
	REPO_PATH="testing/${DISTNAME}/\$basearch"
else
	REPO_PATH="testing/${BUILD}/${DISTNAME}/\$basearch"
fi

cat <<- EOF > "/etc/yum.repos.d/dmc-${BUILD}-${DISTNAME}.repo"
	[dmc-${BUILD}-${DISTNAME}]
	name=DMC Repository
	baseurl=http://dmc-repo.web.cern.ch/dmc-repo/${REPO_PATH}
	gpgcheck=0
	enabled=1
	protect=0
	priority=3
	EOF

echo "dmc-${BUILD}-${DISTNAME}.repo"

# Write FTS dependencies repository file

cat <<- EOF > "/etc/yum.repos.d/fts-depend-${DISTNAME}.repo"
	[fts-depend-${DISTNAME}]
	name=FTS3 Dependencies
	baseurl=https://fts-repo.web.cern.ch/fts-repo/testing/fts-depend/${DISTNAME}/\$basearch
	gpgcheck=0
	enabled=1
	protect=0
	EOF

echo "fts-depend-${DISTNAME}.repo"
