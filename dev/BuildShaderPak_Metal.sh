#!/bin/sh

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

if [ -z "$1" ] || [ -z "$2" ]; then
    echo Missing one or more required params. Example: BuildShaderPak_Metal.sh gameProjectName platform
    echo ""
    echo positional arguments:
    echo "   gameProjectName     The name of the game."
    echo "   platform            ios or osx_gl."
    echo ""
    exit 1
fi

gamename="$1"
platform="$2"
shaderflavor="metal"

if [ "$platform" != "ios" ] && [ "$platform" != "osx_gl" ]; then
    echo Incorrect platform. Example: BuildShaderPak_Metal.sh gameProjectName platform
    echo ""
    echo positional arguments:
    echo "   gameProjectName     The name of the game."
    echo "   platform            ios or osx_gl."
    echo ""
    exit 1
fi

# ShaderCacheGen.exe is not currently able to generate the metal shader cache,
# so for the time being we must copy the compiled shaders from the iOS device or Mac app.
# AppData/Documents/shaders/cache/* -> Cache\MyProject\Platform\user\cache\shaders\cache\*
#
# One ShaderCacheGen.exe can generate the metal shaders (https://issues.labcollab.net/browse/LMBR-18201),
# we should be able to call this directly (after copying the appropriate shaderlist.txt as is done in BuildShaderPak_DX11.bat)
# Bin64\ShaderCacheGen.exe /BuildGlobalCache /ShadersPlatform=%targetplatform%

sh ./lmbr_pak_shaders.sh "$shaderflavor" $gamename $platform

if [ $? -eq 0 ]; then
    echo ---- Process succeeded ----
    exit 0
else
    echo ---- Process failed ----
    exit 1
fi
