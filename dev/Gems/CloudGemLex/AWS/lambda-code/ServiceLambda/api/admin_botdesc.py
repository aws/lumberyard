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
import lex

@service.api
def put(request, bot_desc):
    return {
        'status' : lex.process_bot_desc(bot_desc.get('desc_file', ''))
    }

@service.api
def get(request, bot_name):
    return {
        'desc_file' : lex.get_bot_desc(bot_name)
    }

