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
pushd_silent()
{
    pushd "$@" > /dev/null
}

popd_silent()
{
    popd "$@" > /dev/null
}

TIME="`date +%Y%m%d%H%M%S`"
export COMMAND_ID="$COMPUTERNAME-$TIME"
if [ -n "$BUILD_TAG" ]; then
    if [ -n "$P4_CHANGELIST" ]; then
        export COMMAND_ID="$BUILD_TAG.$P4_CHANGELIST-$TIME"
    fi
fi

pushd_silent $(dirname "$0")

# Extract an optional external engine path if present, otherwise use the cwd as the engine dir
EXTERNAL_ENGINE_PATH=`cat engine.json | grep "ExternalEnginePath" | awk -F":" '{ print $2 }' | sed "s/,//g" | sed "s/\"//g" | xargs echo -n`
if [ -z $EXTERNAL_ENGINE_PATH ]; then
    ENGINE_DIR=$(dirname "$0")
elif [ -d $EXTERNAL_ENGINE_PATH ]; then
    ENGINE_DIR=$EXTERNAL_ENGINE_PATH
else
    echo External Path in engine.json "$EXTERNAL_ENGINE_PATH" does not exist
    exit 1
fi

env python "$ENGINE_DIR/Tools/build/waf-1.7.13/lmbr_waf" "$@"

RESULT=$?

pushd_silent $ENGINE_DIR

if [ -f "Tools/build/waf-1.7.13/build_metrics/build_metrics_overrides.py" ]; then
    if [ $RESULT -eq 0 ]; then
        env python Tools/build/waf-1.7.13/build_metrics/write_build_metric.py WafBuildResult 1 Unitless $*
        if [ -f "build_metrics.txt" ]; then
            env sed -i -e 's/#BUILD_RESULT#/True/g' build_metrics.txt
        fi
    else
        if [ -f "build_metrics.txt" ]; then
            env python Tools/build/waf-1.7.13/build_metrics/write_build_metric.py WafBuildResult 0 Unitless $*
            env sed -i -e 's/#BUILD_RESULT#/False/g' build_metrics.txt
        fi
    fi
fi

popd_silent
popd_silent
exit $RESULT