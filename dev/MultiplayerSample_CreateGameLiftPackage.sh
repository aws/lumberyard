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
#
#!/bin/sh

echo "====================================================="
echo "This is a sample script to create a GameLift package."
echo "You may need to adapt it for your own use."
echo "====================================================="
echo ""

if [ -d GameLiftPackageLinux ]
then
    echo "Cleaning directory GameLiftPackageLinux"
    rm -rf GameLiftPackageLinux
fi

echo "Creating directory GameLiftPackageLinux"
mkdir -p GameLiftPackageLinux

echo "Copying required assets"
if [ -d MultiplayerSample_pc_Paks_Dedicated ]
then
    cp -a MultiplayerSample_pc_Paks_Dedicated/* GameLiftPackageLinux/.
elif [ -d multiplayersample_pc_paks ]
then
    cp -a multiplayersample_pc_paks/* GameLiftPackageLinux/.
else
    echo "Failed to find required assets."
    exit 1
fi

echo "Copying Server binaries"
copied_game_binaries=false

echo ".... Looking for binaries in BinLinux64.Release.Dedicated"
if [ -d BinLinux64.Release.Dedicated ];
then
    cp -a BinLinux64.Release.Dedicated/* GameLiftPackageLinux/.
    copied_game_binaries=true
fi

if [ "$copied_game_binaries" == false ]; 
then
    echo ".... Looking for binaries in BinLinux64.Performance.Dedicated"
    if [ -d BinLinux64.Performance.Dedicated ]
    then
       cp -a BinLinux64.Performance.Dedicated/* GameLiftPackageLinux/.
       copied_game_binaries=true
    fi
fi

if [ "$copied_game_binaries" = false ]; 
then
    echo ".... Looking for binaries in BinLinux64.Dedicated"
    if [ -d BinLinux64.Dedicated ]
    then
        cp -a BinLinux64.Dedicated/* GameLiftPackageLinux/.
        copied_game_binaries=true
    fi
fi

if [ "$copied_game_binaries" == false ]; 
then
    echo "Failed to find game binaries"
    exit 2
else 
    echo ".... Server binaries copied"
fi

echo "Copying libc++ and libc++abi libs"
if [ -f /usr/lib/x86_64-linux-gnu/libc++.a ]
then
    mkdir -p GameLiftPackageLinux/lib64
    cp -a /usr/lib/x86_64-linux-gnu/libc++* GameLiftPackageLinux/lib64/.
else
    echo "Failed to find libc++ and lib++abi libs"
    exit 3
fi 

echo "Copying ca-certificates.crt"
if [ -f /etc/ssl/certs/ca-certificates.crt ]
then
    cp -a /etc/ssl/certs/ca-certificates.crt GameLiftPackageLinux/.
else
    echo "Failed to find and copy /etc/ssl/certs/ca-certificates.crt"
    echo "Without a valid root certificate, your server may not be able to validate and access AWS endpoints. If you see Curl error code 77 in your server logs, this is a likely cause."
fi 

echo "Creating install.sh"
echo "#!/bin/sh" >> GameLiftPackageLinux/install.sh
echo "" >> GameLiftPackageLinux/install.sh
echo "sudo cp -a ./lib64/* /lib64/." >> GameLiftPackageLinux/install.sh

# the following line is optional so we ignore the copy command error code 
echo "sudo cp ./ca-certificates.crt /etc/ssl/certs/ca-certificates.crt || :" >> GameLiftPackageLinux/install.sh


# Extra Setup for GameLift Amazon Linux 2 instace building on Ubuntu 18.04. AL2 has glibc-2.26 by default. Ubuntu 18.04 uses glibc-2.27 by default.
# Compiling and Installing glibc-2.27. Development tools installs dependencies required to install glibc.


. /etc/os-release
if [[ $VERSION == *"18.04"* ]]
then
    echo "if cat /etc/system-release | grep -qFe 'Amazon Linux release 2'" >> GameLiftPackageLinux/install.sh
    echo "then" >> GameLiftPackageLinux/install.sh
    echo " echo 'Installing missing libs for AL2'" >> GameLiftPackageLinux/install.sh
    echo " sudo yum groupinstall 'Development Tools' -y" >> GameLiftPackageLinux/install.sh
    echo " mkdir glibc && cd glibc" >> GameLiftPackageLinux/install.sh
    echo " wget http://mirror.rit.edu/gnu/libc/glibc-2.27.tar.gz" >> GameLiftPackageLinux/install.sh
    echo " tar zxvf glibc-2.27.tar.gz" >> GameLiftPackageLinux/install.sh
    echo " mkdir glibc-2.27-build glibc-2.27-install" >> GameLiftPackageLinux/install.sh
    echo " cd glibc-2.27-build" >> GameLiftPackageLinux/install.sh
    echo " /local/game/glibc/glibc-2.27/configure --prefix='/local/game/glibc/glibc-2.27-install'" >> GameLiftPackageLinux/install.sh
    echo " make -j 8" >> GameLiftPackageLinux/install.sh
    echo " make -j 8 install" >> GameLiftPackageLinux/install.sh
    echo " sudo ln -sf /local/game/glibc/glibc-2.27-install/lib/libm.so.6 /local/game/libm.so.6" >> GameLiftPackageLinux/install.sh
    echo "cd /local/game/" >> GameLiftPackageLinux/install.sh
    echo "fi" >> GameLiftPackageLinux/install.sh
fi


echo "echo 'Install Success'" >> GameLiftPackageLinux/install.sh

chmod +x GameLiftPackageLinux/install.sh
chmod 775 -R GameLiftPackageLinux/*

echo "GameLift package created successfully: GameLiftPackageLinux"

