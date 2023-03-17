#!/bin/bash

source ${TOPSHELL}/shell/buildFunc.sh
source ${TOPSHELL}/shell/buildConf.sh

# jetson-nx-jp51-sd-card-image.zip
# deepstream-6.2_6.2.0-1_arm64.deb

JETSON_MEDIA_CONFIG=jetson_media_defconfig

if [ -e /opt/toolchain/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu/bin/aarch64-linux-gnu-g++ ]; then
    JETSON_CROSS_COMPILE=/opt/toolchain/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu/bin/aarch64-linux-gnu-
else
    JETSON_CROSS_COMPILE=aarch64-linux-gnu-
fi

function jetson_clean()
{
    rm -rf jetsonApp
    remove_gitcommit
}

function jetson_media_app()
{
    begin=`get_timestamp`
    type=$(uname)
    distro=`get_linux_distro`
    version=$(get_general_version)
    echo "Platform type: "${type}" "${distro}" "${version}

    print_info "Starting '${SHELL_NAME}'"

    print_info "build ${PROJECT_NAME} project start"

    export SOC=jetson
    export Platform=nvidia
    export APP_NAME=jetsonApp
    export USE_STDCPP_VERSION=-std=gnu++11
    generate_gitcommit

    make ${JETSON_MEDIA_CONFIG} && make ARCH=arm64 CROSS_COMPILE=${JETSON_CROSS_COMPILE} -j$[$(nproc)-1]
    if [ $? -ne 0 ]; then
        error_exit "Unfortunately, build ${PROJECT_NAME} failed"
    fi

    print_info "build ${PROJECT_NAME} project done."
    print_info "Congratulations, the compilation is successful, Modify by [${AUTHOR_NAME}]"

    print_info "Finished '${SHELL_NAME}'"

    end=`get_timestamp`
    second=`expr ${end} - ${begin}`
    min=`expr ${second} / 60`
    echo "It takes "${min}" minutes, and "${second} "seconds"
}
