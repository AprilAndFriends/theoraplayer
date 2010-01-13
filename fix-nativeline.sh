#!/bin/sh
# This script is used to to set
# svn property for newlines to native, so that each platform gets files
# with its native set of newlines.
# Run every now and then when you add new files.
find . \( -name 'LICENSE' -or -name 'INSTALL*' -or -name 'Makefile' -or -name '*.ac' -or -name '*.h' -or -name '*.cpp' -or -name '*.sh' -or -name '*.bat' -or -name '*.c' -or -name '*.txt' -or -name '*.po' -or -name '*.pot' -or -name '*.vcproj' -or -name '*.sln' \) -print -exec dos2unix '{}' ';' -exec svn ps svn:eol-style native '{}' ';'

