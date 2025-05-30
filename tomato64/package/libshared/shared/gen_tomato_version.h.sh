#!/bin/bash

set -e
set -x

VERSION=$(cat $BR2_EXTERNAL_TOMATO64_PATH/version)
MAJOR=${VERSION%.*}
MINOR=${VERSION#*.}

COMMIT=$(unset GIT_DIR && git -C $BR2_EXTERNAL_TOMATO64_PATH rev-parse HEAD)
TAGS=$(unset GIT_DIR && git -C $BR2_EXTERNAL_TOMATO64_PATH tag --points-at "$COMMIT")

echo "#ifndef __TOMATO_VERSION_H__" >						tomato_version.h
echo "#define __TOMATO_VERSION_H__" >>						tomato_version.h
echo "#define TOMATO_MAJOR		\"$MAJOR\"" >>				tomato_version.h
echo "#define TOMATO_MINOR		\"$MINOR\"" >>				tomato_version.h
echo "#define TOMATO_BUILD		\"00$MINOR\"" >>			tomato_version.h
echo "#define TOMATO_BUILDTIME	\"`date`\"" >>					tomato_version.h

if [ "$PLATFORM_X86_64" == y ]; then
	echo "#define TOMATO_VERSION		\"$VERSION x86_64 AIO\"" >>		tomato_version.h
	echo "$VERSION x86_64 AIO" >>							tomato_version
fi

if [ "$PLATFORM_MT6000" == y ]; then
	echo "#define TOMATO_VERSION		\"$VERSION GL-MT6000 AIO\"" >>		tomato_version.h
	echo "$VERSION GL-MT6000 AIO" >>						tomato_version
fi

if [ "$PLATFORM_BPIR3MINI" == y ]; then
	echo "#define TOMATO_VERSION		\"$VERSION BPI-R3-MINI AIO\"" >>	tomato_version.h
	echo "$VERSION BPI-R3 MINI AIO" >>						tomato_version
fi

echo "#define TOMATO_SHORTVER		\"$VERSION\"" >>			tomato_version.h

if [ ! -n "$TAGS" ]; then

	DATE=$(date -d "yesterday" +%m.%d.%y 2>/dev/null)

	echo "#define TOMATO_NIGHTLY		\" Nightly $DATE\"" >>		tomato_version.h
else
	echo "#define TOMATO_NIGHTLY		\"\"" >>		tomato_version.h
fi
echo "#endif" >>								tomato_version.h
