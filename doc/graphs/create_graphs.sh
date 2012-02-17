#! /bin/bash

ACTUAL_IMAGE=thread_design.txt

IMAGES="$ACTUAL_IMAGE"

java -jar plantuml.jar $IMAGES
