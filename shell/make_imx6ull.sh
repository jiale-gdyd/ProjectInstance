#!/bin/bash

source ${TOPSHELL}/shell/buildFunc.sh
source ${TOPSHELL}/shell/buildConf.sh

if [ -e /opt/toolchain/gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-g++ ]; then
    IMX6ULL_CROSS_COMPILE=/opt/toolchain/gcc-arm-10.3-2021.07-x86_64-arm-none-linux-gnueabihf/bin/arm-none-linux-gnueabihf-
else
    IMX6ULL_CROSS_COMPILE=arm-none-linux-gnueabihf-
fi

function imx6ull_clean()
{
    remove_gitcommit
}
