#!/bin/bash

# Script to install optee
# bases on installation guidelines of 
# https://optee.readthedocs.io/building/gits/build.html#get-and-build-the-solution
# http://mahadevrahul.blogspot.com/2016/02/how-to-set-up-and-run-optee-on-qemu.html

# Its divided in the following sections: preqrequisites, install_repo, get_source_code, get_toolchains, build_solution.
# Each section can be called with source `~/path/to/script/install-optee sectionName`.
# Due to dependencies the sections should be called in the order above.
# Furthermore only one section at once, since some section require parameters.

prerequisites(){
    echo "Installing prerequisites for optee..."
    sudo apt-get install android-tools-adb android-tools-fastboot autoconf \
        automake bc bison build-essential cscope curl device-tree-compiler \
        expect flex ftp-upload gdisk acpica-tools libattr1-dev libdb1-compat tzdata libcap-dev \
        libfdt-dev libftdi-dev libglib2.0-dev libhidapi-dev libncurses5-dev \
        libpixman-1-dev libssl-dev libtool make \
        mtools netcat python-crypto python-serial python-wand unzip uuid-dev \
        xdg-utils xterm xz-utils zlib1g-dev ccache repo || exit 1
}

install_repo(){
    echo "Installing android repo..."
    cd ~ || exit 1
    mkdir -p bin || exit 1
    files='bashrc profile'
    for file in $files
    do
	    if [ -z "$(grep -e ~/bin -e '~/bin' -e '$HOME/bin' .$file)" ]
	    then
	        echo 'export PATH=$HOME/bin:$PATH' >> ~/.$file || exit 1
	    fi
    done
    if [ -z "$(echo "$PATH" | grep -e ~/bin -e '~/bin' -e '$HOME/bin')" ]
    then
	    export "PATH=$HOME/bin:$PATH" # TODO: fix; does not work
    fi
    curl https://storage.googleapis.com/git-repo-downloads/repo > ~/bin/repo || exit 1
    chmod a+x ~/bin/repo || exit 1

}


# $2 is the TARGET and $3 is the branch
# see: https://optee.readthedocs.io/building/gits/build.html#current-version for targets
# or: https://github.com/OP-TEE/manifest for available actualized targets
# and 3.5.0 as branch (latest); do not use 3.3; it is broken!
# QEMU v7 is Armv7-A (32bit), QEMU v8 s Armv8-A (64/32bit)
get_source_code(){
    echo "Getting source code..."
    TARGET=${commandline_args[1]}.xml
    BRANCH=${commandline_args[2]}
    cd ~ || exit 1
    # repo sync not needed for the moment, might be useful later during the TA development
    mkdir -p optee-project || exit 1
    cd optee-project  || exit 1
    repo init -u https://github.com/OP-TEE/manifest.git -m "$TARGET" -b "$BRANCH" || exit 1 # optee-project dir is still empty, just sets up a meta folder .repo that contains references to all subprojects
    repo sync -j4 --no-clone-bundle || exit 1 # optee-project dir is not empty anylonger, actually downloads the git repositories mentioned in the repo manifest file
    # your name: piachristel
    # your email: muellerchristina@bluewin.ch
}

get_toolchains(){
    echo "Getting toolchains..."
    cd ~/optee-project/build || exit 1
    make -j2 toolchains
}

build_solution(){
    echo "Building..."
    cd ~/optee-project/build || exit 1
    make -j$(nproc) || exit 1 # nproc speeds up compilation
}

echo "Downloading and updating packages..."
sudo apt-get update -qq || exit 1

commandline_args=("$@")
"$1" # first argument is function to call
exit 0
