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
import boto3
import CloudCanvas
import errors
import service
from random import randint
import uuid

@service.api(logging_filter=account_utils.apply_logging_filter)
def get(request):
    if not request.event["cognitoIdentityId"]:
        raise errors.ForbiddenRequestError('The credentials used did not contain Cognito identity information')

    account = account_utils.get_account_for_identity(request.event["cognitoIdentityId"])
    if not account:
        raise errors.ClientError("No account found for '{}'".format(request.event["cognitoIdentityId"]))

    if account.get('AccountBlacklisted'):
        raise errors.ForbiddenRequestError('The account is blacklisted')

    response = account_utils.get_account_table().get_item(Key = { 'AccountId': account['AccountId'] }, ConsistentRead = False)

    if not 'Item' in response:
        raise errors.ClientError("No account found for '{}'".format(account['AccountId']))

    return account_utils.convert_account_from_dynamo_to_player_model(response['Item'])

@service.api(logging_filter=account_utils.apply_logging_filter)
def put(request, UpdateAccountRequest):
    if not request.event["cognitoIdentityId"]:
        raise errors.ForbiddenRequestError('The credentials used did not contain Cognito identity information')

    account_utils.validate_account_update_request(UpdateAccountRequest)

    account = account_utils.get_account_for_identity(request.event["cognitoIdentityId"])

    if not account:
        account = {
            'AccountId': str(uuid.uuid4()),
            'CognitoIdentityId': request.event["cognitoIdentityId"]
        }

        if 'PlayerName' in UpdateAccountRequest:
            # IndexedPlayerName and PlayerNameSortKey are used to search accounts by name, case insensitive.  See accountsearch.py
            account['PlayerName'] = UpdateAccountRequest['PlayerName']
            account['IndexedPlayerName'] = UpdateAccountRequest['PlayerName'].lower()
            account['PlayerNameSortKey'] = randint(1, account_utils.get_name_sort_key_count())

        account_utils.create_account(account)
        return account_utils.convert_account_from_dynamo_to_player_model(account)
    else:
        if account.get('AccountBlacklisted'):
            raise errors.ForbiddenRequestError('The account is blacklisted')

        updateRequest = { 'AccountId': account['AccountId'] }

        if 'PlayerName' in UpdateAccountRequest:
            # IndexedPlayerName and PlayerNameSortKey are used to search accounts by name, case insensitive.  See accountsearch.py
            updateRequest['PlayerName'] = UpdateAccountRequest['PlayerName']
            updateRequest['IndexedPlayerName'] = UpdateAccountRequest['PlayerName'].lower()
            updateRequest['PlayerNameSortKey'] = randint(1, account_utils.get_name_sort_key_count())

        result = account_utils.update_account(updateRequest)
        return account_utils.convert_account_from_dynamo_to_player_model(result)
