#!/bin/sh

# This script writes version information in a file to be included by the base.
# You may change variables below to match your project environment.

# Path to destination include file (will be created or overwritten). Update this
# if you renamed the project folder.
PROJECT_VERSION_FILE="src/zr/base/vcsversion.inc"

# Command for printing date.
DATE_COMMAND="date"

# Current revision number in the VCS. Change this if you use something else
# than Mercurial.
PROJECT_REVISION=$(hg id -n):$(hg id -i)

# ------------------------ Do not edit below this line -------------------------

# Whether this is an unofficial build (to prepare distributed source code).
UNOFFICIAL=false

if [ "$1" ]
then
    if [ "$1" = "--unofficial" ]
    then
        UNOFFICIAL=true
    else
        DATE_COMMAND=$1
    fi
fi

if [ $UNOFFICIAL = "true" ]
then
    PROJECT_REVISION="Unofficial build - based on $PROJECT_REVISION"
fi

DATE=$($DATE_COMMAND)

echo "#define PROJECT_VCS_REVISION          \"$PROJECT_REVISION\"" > $PROJECT_VERSION_FILE
echo "#define PROJECT_VCS_DATE              \"$DATE\"" >> $PROJECT_VERSION_FILE

echo "Updated $PROJECT_VERSION_FILE"
