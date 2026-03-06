#!/bin/sh

set -e
set -x

$BR2_EXTERNAL_TOMATO64_PATH/board/common/post-build-fs.sh
