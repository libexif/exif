#!/bin/sh
# Checks that the --version output is sane

. ./check-vars.sh

$EXIFEXE --version | grep '^[0-9]\+\.[0-9]\+[0-9.]*$'
