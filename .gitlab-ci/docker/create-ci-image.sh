#!/bin/sh

echo "Building fts3:ci..."
docker build -t gitlab-registry.cern.ch/fts/fts3:ci .
docker login gitlab-registry.cern.ch/fts/fts3
docker push gitlab-registry.cern.ch/fts/fts3:ci
