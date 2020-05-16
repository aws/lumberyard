#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
#!/bin/sh


# Install updates
apt-get update -y

# Add Mozilla Security repo (needed to install binutils-2.26)
add-apt-repository -y ppa:ubuntu-mozilla-security/ppa
apt-get update -y
apt-get upgrade -y

# Install software
apt-get install -y
apt-get install -y gcc # Internal Developers Only
apt-get install -y binutils-2.26
apt-get install -y libc++-dev # required for clang to work
apt-get install -y libc++abi-dev
apt-get install -y clang
apt-get install -y uuid-dev
apt-get install -y python-tk
# apt-get install -y qt-sdk
apt-get install -y openjdk-7-jre
apt-get install -y libncurses5-dev
apt-get install -y helix-cli
apt-get install -y libcurl3
apt-get install -y libcurl4-openssl-dev
apt-get install -y python-pip
apt-get install -y zlib1g-dev # required for AZCG
apt-get install -y libjpeg-dev
apt-get install -y libjbig-dev
apt-get install -y libsdl2-dev # Needed for WWise
apt-get install -y libcurl3-nss # required by libaws-cpp-sdk-core.so
pip install awscli

# Graphics packages
apt-get install -y libx11-dev
apt-get install -y mesa-common-dev

# Qt 5.12 dependencies
apt-get install -y libopus-dev
apt-get install -y libwebpmux3
apt-get install -y libwebpdemux2
apt-get install -y libwebp-dev

# Create a symlink to libGL and libEGL so we can link with them.
ln -s /usr/lib/x86_64-linux-gnu/libGL.so.1 /usr/lib/libGL.so
ln -s /usr/lib/x86_64-linux-gnu/libEGL.so.1 /usr/lib/libEGL.so

# Configuration
# mv /usr/bin/ld /usr/bin/ld.bak
# mv /usr/bin/ld-2.26 /usr/bin/ld
ln -s /usr/bin/clang-3.8 /usr/bin/clang
ln -s /usr/bin/clang++-3.8 /usr/bin/clang++
# cp -v /usr/include/libcxxabi/* /usr/include/c++/v1/
# hash -r # This needs to be run from the parent shell and not part of this script.

echo
echo
echo "=============================|======================|============================"
echo "Name                         | Expected Version     | Installed Version          "
echo "                             |    (or Higher)       |                            "
echo "=============================|======================|============================"
echo "binutils                     | 2.26.1               | `dpkg -l binutils | grep -E "^ii" | tr -s ' ' | cut -d' ' -f3`"
echo "clang-3.8                    | 3.8                  | `dpkg -l clang-3.8 | grep -E "^ii" | tr -s ' ' | cut -d' ' -f3`"
echo "g++                          | 5.3.1                | `dpkg -l g++ | grep -E "^ii" | tr -s ' ' | cut -d' ' -f3`"
echo "gcc                          | 5.3.1                | `dpkg -l gcc | grep -E "^ii" | tr -s ' ' | cut -d' ' -f3`"
echo "libc++-dev                   | 3.7.0                | `dpkg -l libc++-dev | grep -E "^ii" | tr -s ' ' | cut -d' ' -f3`"
echo "libc++abi-dev                | 3.7.0                | `dpkg -l libc++abi-dev | grep -E "^ii" | tr -s ' ' | cut -d' ' -f3`"
echo "libcurl4-openssl-dev         | 7.47.0               | `dpkg -l libcurl4-openssl-dev | grep -E "^ii" | tr -s ' ' | cut -d' ' -f3`"
echo "libncurses5-dev              | 6.0                  | `dpkg -l libncurses5-dev | grep -E "^ii" | tr -s ' ' | cut -d' ' -f3`"
echo "python-pip                   | 8.1.1                | `dpkg -l python-pip | grep -E "^ii" | tr -s ' ' | cut -d' ' -f3`"
echo "python-tk                    | 2.7.11               | `dpkg -l python-tk | grep -E "^ii" | tr -s ' ' | cut -d' ' -f3`"
echo "ubuntu-release-upgrader-core | 16.04.22             | `dpkg -l ubuntu-release-upgrader-core | grep -E "^ii" | tr -s ' ' | cut -d' ' -f3`"
echo "uuid-dev                     | 2.27.1               | `dpkg -l uuid-dev | grep -E "^ii" | tr -s ' ' | cut -d' ' -f3`"
echo "libopus-dev                  | 1.1.2-1ubuntu1       | `dpkg -l libopus-dev | grep -E "^ii" | tr -s ' ' | cut -d' ' -f3`"
echo "libwebpmux3                  | 0.6.1-2              | `dpkg -l libwebpmux3 | grep -E "^ii" | tr -s ' ' | cut -d' ' -f3`"
echo "libwebpdemux2                | 0.6.1-2              | `dpkg -l libwebpdemux2 | grep -E "^ii" | tr -s ' ' | cut -d' ' -f3`"
echo "libwebp-dev                  | 0.6.1-2              | `dpkg -l libwebp-dev | grep -E "^ii" | tr -s ' ' | cut -d' ' -f3`"
echo "=============================|======================|============================"
echo
echo
