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
#

shader_type="$1"
game="$2"
platform="$3"
if [ -z "$platform" ]; then
    platform=pc
fi
source="Cache/$game/$platform/user/cache/shaders/cache"
output="Build/$platform/$game"

env python Tools/PakShaders/pak_shaders.py $output -r $source -s $shader_type
