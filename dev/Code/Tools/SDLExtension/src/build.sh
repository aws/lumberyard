#!/bin/bash

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
# Original file Copyright Crytek GMBH or its affiliates, used under license.
#

# Create a symbolic link to Code/SDKs/SDL2. (symbolic link created during bootstraping)

ndk-build NDK_MODULE_PATH=.:../../../SDKs/ APP_PLATFORM=android-19 APP_ABI=armeabi-v7a APP_OPTIM=release

cp libs/armeabi-v7a/libSDL2Ext.so ../lib/android-armeabi-v7a/libSDL2Ext.so
