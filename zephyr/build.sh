#!/bin/bash -e

export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
export GNUARMEMB_TOOLCHAIN_PATH=/opt/gcc-arm-none-eabi-9-2020-q2-update

ROOT=$PWD
NAME=$0
ARGV=$*

if [ -d zephyr ]; then
	cd zephyr
fi

source ./zephyr-env.sh
python ${NAME%.*}.py $ARGV

cd $ROOT
