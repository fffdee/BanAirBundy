#!/bin/bash
export PATH=/cygdrive/C/Andestech/AndeSight300RDS/cygwin/bin:/cygdrive/C/Andestech/AndeSight300RDS/toolchains/nds32le-elf-mculib-v3s/bin:$PATH
cd /cygdrive/D/BanAirBundy/boot_app
sed -i -E "s/(#define BOOT_APP_WIRELESS_ROLE_TX[[:space:]]+)[01]/\1$1/" system_config/app_config.h
echo "ROLE_TX set to $1"
cd Debug
make all > /cygdrive/D/BanAirBundy/build_$1.log 2>&1
if [ "$1" = "1" ]; then cp output/boot_app.bin output/tx.bin; else cp output/boot_app.bin output/rx.bin; fi
echo "BUILD_DONE role=$1"
