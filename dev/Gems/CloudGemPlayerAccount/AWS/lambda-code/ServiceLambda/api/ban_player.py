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
import CloudCanvas
import admin_accounts

@service.api
def post(request, request_data):
    account = get_player_account(request_data["id"])
    if account == None:
        return { "status": "player not found"}

    do_ban_operation(account['AccountId'], True)
    return { "status": "BANNED" }

@service.api
def delete(request, request_data):
    account = get_player_account(request_data["id"])
    if account == None:
        return { "status": "player not found"}

    do_ban_operation(account['AccountId'], False)
    return { "status": "UNBANNED" }


def do_ban_operation(acc_id, ban):
    update_player_account(acc_id, { "AccountBlacklisted": ban })

def get_player_account(cog_id):
    return account_utils.get_account_for_identity(cog_id)

def update_player_account(acc_id, model):
    admin_accounts.put_account(acc_id, model, create_account=False)
