#!/bin/bash

source ${TOPSHELL}/shell/buildFunc.sh
source ${TOPSHELL}/shell/buildConf.sh

AX620A_MEDIA_CONFIG=ax620a_media_defconfig

if [ -e /opt/toolchain/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++ ]; then
    AX620A_CROSS_COMPILE=/opt/toolchain/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
else
    AX620A_CROSS_COMPILE=arm-linux-gnueabihf-
fi

function ax620a_clean()
{
    rm -rf ax620aApp
    remove_gitcommit
}

function ax620a_media_app()
{
    begin=`get_timestamp`
    type=$(uname)
    distro=`get_linux_distro`
    version=$(get_general_version)
    echo "Platform type: "${type}" "${distro}" "${version}

    print_info "Starting '${SHELL_NAME}'"

    print_info "build ${PROJECT_NAME} project start"

    export SOC=ax620a
    export Platform=axera
    export APP_NAME=ax620aApp
    export USE_STDCPP_VERSION=-std=gnu++17
    generate_gitcommit

    make ${AX620A_MEDIA_CONFIG} && make ARCH=arm CROSS_COMPILE=${AX620A_CROSS_COMPILE} -j$[$(nproc)-1]
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
