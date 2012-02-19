#! /bin/bash

ACTUAL_IMAGE=common_error.txt

#IMAGES="common_logger.txt common_synchronization_framework.txt $ACTUAL_IMAGE"

java -jar plantuml.jar $ACTUAL_IMAGE
