#!/usr/bin/env bash
set -e

function print_info {
  printf "======================\n"
  printf "%-16s%s\n" "Distribution:" "${DIST}"
  printf "%-16s%s\n" "Dist name:" "${DISTNAME}"
  printf "%-16s%s\n" "Build type:" "${BUILD}"
  printf "%-16s%s\n" "Branch:" "${BRANCH}"
  printf "%-16s%s\n" "Release:" "${RELEASE}"
  printf "%-16s%s\n" "DMC Repository:" "${REPO_FILE}"
  printf "======================\n"
}

TIMESTAMP=$(git log -1 --format="%at" | xargs -I{} date -d @{} +%y%m%d%H%M)
GITREF=`git rev-parse --short=7 HEAD`
RELEASE=r${TIMESTAMP}git${GITREF}
BUILD="devel"

if [[ -z ${BRANCH} ]]; then
  BRANCH=`git name-rev $GITREF --name-only`
else
  printf "Using environment set variable BRANCH=%s\n" "${BRANCH}"
fi

if [[ $BRANCH =~ ^(tags/)?(v)[.0-9]+(-(rc)?([0-9]+))?(\^0)?$ ]]; then
  RELEASE="${BASH_REMATCH[4]}${BASH_REMATCH[5]}"
  BUILD="rc"
fi

DIST=$(rpm --eval "%{dist}" | cut -d. -f2)
DISTNAME=${DIST}

# Write repository files to /etc/yum.repos.d/ based on the branch name
REPO_FILE=$(./ci/write-repo-file.sh)
print_info

RPMBUILD=${PWD}/build
SRPMS=${RPMBUILD}/SRPMS

cd packaging/
make srpm RELEASE=${RELEASE} RPMBUILD=${RPMBUILD} SRPMS=${SRPMS}
dnf builddep -y ${SRPMS}/*

rpmbuild --rebuild --define="_topdir ${RPMBUILD}" ${SRPMS}/*
