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

echo "Copying game binaries"
if [ -d BinLinux64.Dedicated ]
then
    cp -a BinLinux64.Dedicated/* GameLiftPackageLinux/.
else
    echo "Failed to find game binaries"
    exit 2
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

chmod +x GameLiftPackageLinux/install.sh

echo "GameLift package created successfully: GameLiftPackageLinux"

