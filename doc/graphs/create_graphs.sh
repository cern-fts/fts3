#! /bin/bash

ACTUAL_IMAGE=synchronization_framework.txt

IMAGES="$ACTUAL_IMAGE"

java -jar plantuml.jar $IMAGES
