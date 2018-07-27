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

if [ -z "$1" ]; then
    echo Missing one or more required arguments. Example: BuildShaderPak_ES3.sh gameProjectName
    echo ""
    echo positional arguments:
    echo "   gameProjectName     The name of the game."
    echo ""
    exit 1
fi

platform="es3"
shaderflavor="gles3*"
gamename="$1"

sh ./lmbr_pak_shaders.sh "$shaderflavor" $gamename $platform

if [ $? -eq 0 ]; then
    echo ---- Process succeeded ----
    exit 0
else
    echo ---- Process failed ----
    exit 1
fi
