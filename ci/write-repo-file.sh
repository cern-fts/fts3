#!/usr/bin/env bash
set -e

DIST=$(rpm --eval "%{dist}" | cut -d. -f2)
DISTNAME=${DIST}

# Special handling of FC rawhide
[[ "${DISTNAME}" == "fc35" ]] && DISTNAME="fc-rawhide"
[[ "${DISTNAME}" == "fc36" ]] && DISTNAME="fc-rawhide"

if [[ -z ${DMC_REPO} ]]; then
  REPO="develop"
  REPO_PATH="testing/${DISTNAME}/\$basearch"
else
  REPO=${DMC_REPO}
  REPO_PATH="testing/${REPO}/${DISTNAME}/\$basearch"
fi

echo "Installing /etc/yum.repos.d/dmc-${REPO}-${DISTNAME}.repo"
cat <<- EOF > "/etc/yum.repos.d/dmc-${REPO}-${DISTNAME}.repo"
	[dmc-${REPO}-${DISTNAME}]
	name=DMC Repository
	baseurl=http://dmc-repo.web.cern.ch/dmc-repo/${REPO_PATH}
	gpgcheck=0
	enabled=1
	protect=0
	priority=3
	EOF
