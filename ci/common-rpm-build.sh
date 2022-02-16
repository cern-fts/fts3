#!/usr/bin/env bash
set -e

function print_info {
  printf "======================\n"
  printf "Distribution:\t%s\n" "${DIST}"
  printf "Dist name:\t%s\n" "${DISTNAME}"
  printf "Build type:\t%s\n" "${BUILD}"
  printf "Branch:\t\t%s\n" "${BRANCH}"
  printf "Release:\t%s\n" "${RELEASE}"
  printf "DMC Repository:\t%s\n" "${REPO_FILE}"
  printf "======================\n"
}

TIMESTAMP=`date +%y%m%d%H%M`
GITREF=`git rev-parse --short HEAD`
RELEASE=r${TIMESTAMP}git${GITREF}
BUILD="devel"

if [[ -z ${BRANCH} ]]; then
  BRANCH=`git name-rev $GITREF --name-only`
else
  printf "Using environment set variable BRANCH=%s\n" "${BRANCH}"
fi

if [[ $BRANCH =~ ^(tags/)?(v)[.0-9]+(-[0-9]+)?$ ]]; then
  RELEASE=
  BUILD="rc"
fi

DIST=$(rpm --eval "%{dist}" | cut -d. -f2)
DISTNAME=${DIST}

# Fetch repository files from fts/build-utils
./ci/fetch-repo-files.sh

REPO_FILE="${BUILD}/dmc-${BUILD}-${DISTNAME}.repo"
print_info

if [[ -f "ci/repo/${REPO_FILE}" ]]; then
  cp -v "ci/repo/${REPO_FILE}" "/etc/yum.repos.d/"
fi

RPMBUILD=${PWD}/build
SRPMS=${RPMBUILD}/SRPMS

cd packaging/
make srpm RELEASE=${RELEASE} RPMBUILD=${RPMBUILD} SRPMS=${SRPMS}

if [[ -f /usr/bin/dnf ]]; then
  dnf install -y epel-release || true
  dnf builddep -y ${SRPMS}/*
else
  yum-builddep -y ${SRPMS}/*
fi

rpmbuild --rebuild --define="_topdir ${RPMBUILD}" ${SRPMS}/*
