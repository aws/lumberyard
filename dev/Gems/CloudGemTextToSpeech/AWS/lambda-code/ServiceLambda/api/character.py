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
import character_config

@service.api
def get(request, name):
    return character_config.get_character_info(name)

@service.api
def delete(request, name):
    character_config.delete_character(name)
    return { "status": "success" }


@service.api
def post(request, char_def):
    return character_config.add_character(char_def)