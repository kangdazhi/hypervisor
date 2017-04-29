#!/bin/bash -e
#
# Bareflank Hypervisor
#
# Copyright (C) 2015 Assured Information Security, Inc.
# Author: Rian Quinn        <quinnr@ainfosec.com>
# Author: Brendan Kerrigan  <kerriganb@ainfosec.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

source /home/user/hypervisor/env.sh

if [[ ! -d "$BUILD_ABS/source_libcxx" ]]; then
    $BUILD_ABS/build_scripts/fetch_libcxx.sh $BUILD_ABS
fi

if [[ ! -d "$BUILD_ABS/source_libcxxabi" ]]; then
    $BUILD_ABS/build_scripts/fetch_libcxxabi.sh $BUILD_ABS
fi

if [[ ! -d "$BUILD_ABS/source_llvm" ]]; then
    $BUILD_ABS/build_scripts/fetch_llvm.sh $BUILD_ABS
fi

rm -Rf $BUILD_ABS/build_libcxx
rm -Rf $BUILD_ABS/sysroot_$SYSROOT_NAME/x86_64-$SYSROOT_NAME-elf/include/c++/
mkdir -p $BUILD_ABS/build_libcxx

pushd $BUILD_ABS/build_libcxx

if [[ $PRODUCTION == "yes" ]]; then
    BUILD_TYPE=Release
else
    BUILD_TYPE=Debug
fi

export CFLAGS="$CFLAGS $CROSS_CCFLAGS"
export CXXFLAGS="$CXXFLAGS $CROSS_CXXFLAGS"

cmake $BUILD_ABS/source_libcxx/ \
    -DCMAKE_SYSTEM_NAME=Linux \
    -DLLVM_PATH=$BUILD_ABS/source_llvm \
    -DLIBCXX_CXX_ABI=libcxxabi \
    -DLIBCXX_CXX_ABI_INCLUDE_PATHS=$BUILD_ABS/source_libcxxabi/include \
    -DCMAKE_INSTALL_PREFIX=$BUILD_ABS/sysroot_$SYSROOT_NAME/x86_64-$SYSROOT_NAME-elf/ \
    -DLIBCXX_SYSROOT=$BUILD_ABS/sysroot_$SYSROOT_NAME/x86_64-$SYSROOT_NAME-elf/ \
    -DCMAKE_C_COMPILER=$BUILD_ABS/build_scripts/x86_64-$SYSROOT_NAME-clang \
    -DCMAKE_CXX_COMPILER=$BUILD_ABS/build_scripts/x86_64-$SYSROOT_NAME-clang++ \
    -DCMAKE_AR=$BUILD_ABS/build_scripts/x86_64-$SYSROOT_NAME-elf-ar \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DLIBCXX_HAS_PTHREAD_API=ON \
    -DLIBCXX_ENABLE_EXPERIMENTAL_LIBRARY=OFF

make -j2
make -j2 install

popd