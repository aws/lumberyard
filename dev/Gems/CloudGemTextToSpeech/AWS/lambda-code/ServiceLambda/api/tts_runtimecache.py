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
import lifecycle_config


@service.api
def post(request, request_info):
    tts.enable_cache_runtime_generated_files(request_info["enabled"])
    lifecycle_config.set_lifecycle_config()
    return { "status": "success" }

@service.api
def get(request):
    if tts.cache_runtime_generated_files():
        return { "status": "enabled" }
    else:
        return { "status": "disabled" }