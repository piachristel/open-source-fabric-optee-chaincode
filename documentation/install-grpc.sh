#!/bin/bash

# Script to cross-compile grpc
# bases on installation guidelines of:
# https://github.com/grpc/grpc/blob/master/BUILDING.md#linux
# https://github.com/grpc/grpc/blob/master/templates/Makefile.template
# https://stackoverflow.com/questions/54052229/build-grpc-c-for-android-using-ndk-arm-linux-androideabi-clang-compiler
# https://github.com/grpc/grpc/issues/9719#issuecomment-284843131
# https://www.mail-archive.com/grpc-io@googlegroups.com/msg04551.html
# https://groups.google.com/forum/#!topic/grpc-io/848KwP13fPc (recommends to compile the gRPC library statically)
# https://groups.google.com/forum/#!topic/grpc-io/848KwP13fPc
# https://github.com/grpc/grpc/issues/6756#issuecomment-228479843

# Its divided in the multiple sections.
# Make script executable by running $chmod a+x install-grpc
# Each section can be called with source `~/path/to/script/install-grpc sectionName`.
# Due to dependencies the sections should be called in the order below.

prerequisites(){
    echo "Installing prerequisites for grpc..."
    sudo apt-get update -q || exit 1
    sudo apt-get install build-essential autoconf libtool pkg-config \
        libgflags-dev libgtest-dev \
        clang libc++-dev \
        autoconf automake libtool curl make g++ unzip \
        g++-aarch64-linux-gnu || exit 1
}

clone_and_init(){
    echo "Cloning grpc repo and init submodules..."
    # see https://github.com/grpc/grpc/blob/master/BUILDING.md#unix for latest stable release 
    # tag v1.20.0, commit 9dfbd34f5c0b20bd77658c73c59b9a3e4e8f4e14
    git clone -b v1.20.0 https://github.com/grpc/grpc || exit 1 
    cd ~/grpc || exit 1
    git submodule update --init || exit 1
}

install_grpc_natively(){
    echo "Install grpc and protobuf natively..."
    grpc_src='/home/ubuntu/grpc' || exit 1
    cd $grpc_src/third_party/protobuf || exit 1
    ./autogen.sh || exit 1
    ./configure || exit 1
    make || exit 1
    sudo make install || exit 1
    sudo ldconfig || exit 1
    cd $grpc_src || exit 1
    make || exit 1
    sudo make install || exit 1
    sudo ldconfig || exit 1
    make plugins || exit 1
}

cross_compile_grpc(){
    echo "Cross-compiling grpc as static library..."
    grpc_src='/home/ubuntu/grpc' || exit 1
    cd $grpc_src || exit 1
    make clean || exit 1
    export GRPC_CROSS_COMPILE=true || exit 1
    export GRPC_CROSS_AROPTS="rc --target=elf32-little" || exit 1
    export HAS_PKG_CONFIG=false || exit 1
    export CC="aarch64-linux-gnu-gcc" || exit 1
    export CXX="aarch64-linux-gnu-g++" || exit 1
    export RANLIB="aarch64-linux-gnu-gcc-ranlib" || exit 1
    export LD="aarch64-linux-gnu-gcc" || exit 1
    export LDXX="aarch64-linux-gnu-g++" || exit 1
    export AR="aarch64-linux-gnu-ar" || exit 1
    export HOST_CC="/usr/bin/gcc" || exit 1
    export HOST_CXX="/usr/bin/g++" || exit 1
    export HOST_LD="/usr/bin/ld" || exit 1
    export STRIP="aarch64-linux-gnu-strip" || exit 1
    export PROTOBUF_CONFIG_OPTS="--host=aarch64-linux-gnu --with-protoc=/usr/local/bin/protoc" || exit 1
    export GRPC_CROSS_LDOPTS="-L/usr/local/lib -L/usr/local/cross/lib" || exit 1
    make static 
}

commandline_args=("$@")
"$1" # first argument is function to call
exit 0
