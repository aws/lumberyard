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

TIME="`date +%Y%m%d%H%M%S`"
BUILD_ID="$COMPUTERNAME.$TIME"

env python Tools/build/waf-1.7.13/lmbr_waf "$@"

RESULT=$?

if [ -f "Tools/build/waf-1.7.13/build_metrics/build_metrics_overrides.py" ]; then
    if [ $RESULT -eq 0 ]; then
        env python Tools/build/waf-1.7.13/build_metrics/write_build_metric.py WafBuildResult 1 Unitless $*
    else
        env python Tools/build/waf-1.7.13/build_metrics/write_build_metric.py WafBuildResult 0 Unitless $*
    fi
fi

exit $RESULT