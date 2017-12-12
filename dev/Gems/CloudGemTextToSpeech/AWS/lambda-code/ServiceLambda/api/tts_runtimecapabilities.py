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

import service
import tts

@service.api
def post(request, request_info):
    tts.enable_runtime_capabilities(request_info["enabled"])
    return { "status": "success" }

@service.api
def get(request):
    if tts.runtime_capabilities_enabled():
        return { "status": "enabled" }
    else:
        return { "status": "disabled" }