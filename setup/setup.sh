#!/bin/bash

set -ex

download_dir="/tmp/ngage-toolchain/downloads"
toolchain_dir="/tmp/ngage-toolchain/toolchain"

apps_url="https://github.com/mupfdev/mupfdev.github.io/raw/refs/heads/main/ngagesdk/apps.zip"
#sdk_url="https://github.com/mupfdev/mupfdev.github.io/raw/refs/heads/main/ngagesdk/sdk.zip"
sdk_url="https://github.com/ngagesdk/sdk/archive/refs/heads/linux.zip"
extras_url="https://github.com/mupfdev/mupfdev.github.io/raw/refs/heads/main/ngagesdk/extras.zip"
tools_url="https://github.com/mupfdev/mupfdev.github.io/raw/refs/heads/main/ngagesdk/tools.zip"

mkdir -p "${download_dir}"

if [ ! -f "${download_dir}/apps.zip" ]; then
  wget "${apps_url}" -O  "${download_dir}/apps.zip"
fi
if [ ! -f "${download_dir}/sdk.zip" ]; then
  wget "${sdk_url}" -O  "${download_dir}/sdk.zip"
fi
if [ ! -f "${download_dir}/tools.zip" ]; then
  wget "${tools_url}" -O  "${download_dir}/tools.zip"
fi
if [ ! -f "${download_dir}/extras.zip" ]; then
  wget "${extras_url}" -O  "${download_dir}/extras.zip"
fi

rm -rf "${toolchain_dir}"
mkdir -p "${toolchain_dir}"

unzip -d "${toolchain_dir}" "${download_dir}/apps.zip"
unzip -d "${toolchain_dir}" "${download_dir}/sdk.zip"
mv "${download_dir}/sdk-linux" "${download_dir}/sdk"

unzip -d "${toolchain_dir}" "${download_dir}/tools.zip"
unzip -d "${toolchain_dir}" "${download_dir}/extras.zip"
