#! /bin/bash

ACTUAL_IMAGE=common_logger.txt

IMAGES="common_synchronization_framework.txt $ACTUAL_IMAGE"

java -jar plantuml.jar $IMAGES
