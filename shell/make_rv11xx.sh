#!/bin/bash

source ${TOPSHELL}/shell/buildFunc.sh
source ${TOPSHELL}/shell/buildConf.sh

RV11XX_MEDIA_CONFIG=rv11xx_media_defconfig

if [ -e /opt/toolchain/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++ ]; then
    RV11XX_CROSS_COMPILE=/opt/toolchain/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
else
    RV11XX_CROSS_COMPILE=arm-linux-gnueabihf-
fi

function rv11xx_clean()
{
    rm -rf rv1126App
    remove_gitcommit
}

function rv11xx_media_app()
{
    begin=`get_timestamp`
    type=$(uname)
    distro=`get_linux_distro`
    version=$(get_general_version)
    echo "Platform type: "${type}" "${distro}" "${version}

    print_info "Starting '${SHELL_NAME}'"

    print_info "build ${PROJECT_NAME} project start"

    export SOC=rv11xx
    export Platform=rockchip
    export APP_NAME=rv1126App
    export USE_STDCPP_VERSION=-std=gnu++11
    generate_gitcommit

    make ${RV11XX_MEDIA_CONFIG} && make ARCH=arm CROSS_COMPILE=${RV11XX_CROSS_COMPILE} -j$[$(nproc)-1]
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
