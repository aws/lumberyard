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

import account_utils
import errors
import service
import admin_accountSearch
import json

@service.api
def get(request):
    full_list =  admin_accountSearch.default_search()
    return {
        "players": [p["CognitoUsername"] for p in full_list["Accounts"] if p.get("CognitoUsername") and p.get('AccountBlacklisted', False)]
    }