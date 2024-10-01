#!/bin/bash

VERSION=$(cat $BR2_EXTERNAL_TOMATO64_PATH/version)
MAJOR=${VERSION%.*}
MINOR=${VERSION#*.}

echo "#ifndef __TOMATO_VERSION_H__" >						tomato_version.h
echo "#define __TOMATO_VERSION_H__" >>						tomato_version.h
echo "#define TOMATO_MAJOR		\"$MAJOR\"" >>				tomato_version.h
echo "#define TOMATO_MINOR		\"$MINOR\"" >>				tomato_version.h
echo "#define TOMATO_BUILD		\"00$MINOR\"" >>			tomato_version.h
echo "#define TOMATO_BUILDTIME	\"`date`\"" >>					tomato_version.h

if [ "$PLATFORM_X86_64" == y ]; then
	echo "#define TOMATO_VERSION		\"$VERSION x86_64 AIO\"" >>		tomato_version.h
fi

if [ "$PLATFORM_MT6000" == y ]; then
	echo "#define TOMATO_VERSION		\"$VERSION GL-MT6000 AIO\"" >>		tomato_version.h
fi

echo "#define TOMATO_SHORTVER		\"$VERSION\"" >>			tomato_version.h
echo "#endif" >>								tomato_version.h

echo "$VERSION" > tomato_version
