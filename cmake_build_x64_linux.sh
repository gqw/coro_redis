#!/bin/bash
cur_dir=$(cd "$(dirname "$0")";pwd)
cd $cur_dir

mkdir -p build_linux
cd build_linux

if [ ! -f $VCPKG_ROOT/vcpkg ]; then
	$VCPKG_ROOT/bootstrap-vcpkg.sh
fi

$VCPKG_ROOT/vcpkg install hiredis:x64-linux
$VCPKG_ROOT/vcpkg install spdlog:x64-linux

echo cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET="x64-linux"
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET="x64-linux"