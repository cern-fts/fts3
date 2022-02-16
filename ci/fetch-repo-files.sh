#!/usr/bin/env bash
set -e

pushd ci > /dev/null
git init --quiet
git remote add --fetch origin "https://gitlab.cern.ch/fts/build-utils.git" &> /dev/null
git config core.sparseCheckout true
echo "repo" >> .git/info/sparse-checkout
git pull --quiet origin master
rm -rf .git
popd > /dev/null
