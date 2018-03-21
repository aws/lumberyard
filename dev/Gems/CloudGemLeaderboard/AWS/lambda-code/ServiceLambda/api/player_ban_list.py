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

import boto3
import cgf_service_client
import cgf_lambda_settings
import service
import ban_handler
import json

@service.api
def get(request):
    interface_url = cgf_lambda_settings.get_service_url("CloudGemPlayerAccount_banplayer_1_0_0")
    if not interface_url:
        return {
            "players": ban_handler.get_banned_players()
        }

    service_client = cgf_service_client.for_url(interface_url, verbose=True, session=boto3._get_default_session())
    result = service_client.navigate('list').GET()
    return result.DATA

