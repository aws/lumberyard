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
from boto3.dynamodb.conditions import Key
import CloudCanvas
import errors
import service
import traceback

@service.api
def get(request, StartPlayerName, CognitoIdentityId, CognitoUsername, Email):
    if CognitoIdentityId:
        return search_by_cognito_identity_id(CognitoIdentityId)
    if CognitoUsername:
        return search_by_username(CognitoUsername)
    if Email:
        return search_by_email(Email)
    if StartPlayerName:
        return search_by_start_name(StartPlayerName)

    return default_search()

def search_by_cognito_identity_id(CognitoIdentityId):
    response = account_utils.get_account_table().query(
        ConsistentRead=False,
        IndexName='CognitoIdentityIdIndex',
        KeyConditionExpression=Key('CognitoIdentityId').eq(CognitoIdentityId)
    )

    accounts = []

    for item in response.get('Items', []):
        accounts.append(account_utils.convert_account_from_dynamo_to_admin_model(item))

    populate_identity_providers(accounts)

    return {'Accounts': accounts}

def search_by_start_name(StartPlayerName):
    sortKeyCount = account_utils.get_name_sort_key_count()
    limit = 20

    accounts = []

    # Accounts are randomly split across multiple partition keys in accounts.py to distribute load on the index.
    # This will collect results from all of the keys and combine them.
    for partition in range(1, sortKeyCount + 1):
        # The query limit is set to the same limit as the number of returned accounts in case the results all come from
        # the same partition key.  An eventually consistent query consumes half a unit per 4KB regardless of how
        # many items contributed to the 4KB, so it's not too inefficient to read a lot of small items and throw most of them away.
        # This implementation expects searches to be relatively infrequent.
        queryArgs = {
            'ConsistentRead': False,
            'IndexName': 'PlayerNameIndex',
            'KeyConditionExpression': Key('PlayerNameSortKey').eq(partition),
            'Limit': limit,
            'ScanIndexForward': True
            }

        if StartPlayerName:
            # Convert the inclusive start key from the request into an exclusive start key since Dynamo only supports exclusive.
            # Dynamo sorts keys by utf-8 bytes.  This attempts to decrement the last byte so that it's one before
            # the start name.  Not guaranteed to work in all cases.
            exclusiveStartKeyBytes = bytearray(StartPlayerName.lower(), 'utf-8')
            if exclusiveStartKeyBytes[-1] > 0:
                exclusiveStartKeyBytes[-1] = exclusiveStartKeyBytes[-1] - 1
            exclusiveStartKey = exclusiveStartKeyBytes.decode('utf-8', 'ignore')

            queryArgs['ExclusiveStartKey'] = {'PlayerNameSortKey': partition, 'IndexedPlayerName': exclusiveStartKey, 'AccountId': ' '}
        response = account_utils.get_account_table().query(**queryArgs)

        for item in response.get('Items', []):
            accounts.append(account_utils.convert_account_from_dynamo_to_admin_model(item))

    accounts.sort(cmp=account_utils.compare_accounts)
    if len(accounts) > limit:
        del accounts[limit:]

    populate_identity_providers(accounts)

    return {'Accounts': accounts}

def search_by_username(Username):
    response = account_utils.get_user_pool_client().list_users(
        UserPoolId=account_utils.get_user_pool_id(),
        Filter='username ^= "{}"'.format(Username.replace('"', '')),
        Limit=20
    )

    users = response.get('Users', [])
    if not users:
        print 'No users found for the requested username prefix.'
        return {'Accounts': []}

    accounts = []

    for user in users:
        accounts.append(populate_account_for_user(user))

    accounts.sort(cmp=account_utils.compare_accounts)

    return {'Accounts': accounts}

def search_by_email(Email):
    response = account_utils.get_user_pool_client().list_users(
        UserPoolId=account_utils.get_user_pool_id(),
        Filter='email ^= "{}"'.format(Email.replace('"', '')),
        Limit=20
    )

    users = response.get('Users', [])
    if not users:
        print 'No users found for the requested email prefix.'
        return {'Accounts': []}

    accounts = []

    for user in users:
        accounts.append(populate_account_for_user(user))

    accounts.sort(cmp=account_utils.compare_accounts)

    return {'Accounts': accounts}

def default_search():
    listUsersResponse = account_utils.get_user_pool_client().list_users(UserPoolId=account_utils.get_user_pool_id(), Limit=50)
    usersByName = {user.get('Username', ''): user for user in listUsersResponse.get('Users', [])}

    accounts = []
    scanResponse = account_utils.get_account_table().scan(Limit=50)
    for accountItem in scanResponse.get('Items', []):
        account = account_utils.convert_account_from_dynamo_to_admin_model(accountItem)
        username = account.get('CognitoUsername')
        if username:
            user = usersByName.get(username)
            if user:
                account['IdentityProviders'] = {
                    account_utils.IDP_COGNITO: account_utils.convert_user_from_cognito_to_model(user)
                }
                del usersByName[username]
            else:
                populate_identity_provider(account)
        accounts.append(account)

    for username, user in usersByName.iteritems():
        account = populate_account_for_user(user)
        accounts.append(account)

    accounts.sort(cmp=account_utils.compare_accounts)

    return {'Accounts': accounts}

def get_accounts_by_username(Username):
    response = account_utils.get_account_table().query(
        ConsistentRead=False,
        IndexName='CognitoUsernameIndex',
        KeyConditionExpression=Key('CognitoUsername').eq(Username)
    )

    accounts = []

    for item in response.get('Items', []):
        accounts.append(account_utils.convert_account_from_dynamo_to_admin_model(item))

    populate_identity_providers(accounts)

    return accounts

def populate_identity_providers(accounts):
    for account in accounts:
        populate_identity_provider(account)

def populate_identity_provider(account):
    if 'CognitoUsername' in account:
        try:
            user = account_utils.get_user_pool_client().admin_get_user(UserPoolId=account_utils.get_user_pool_id(), Username=account['CognitoUsername'])
            if 'IdentityProviders' not in account:
                account['IdentityProviders'] = {}
            account['IdentityProviders'][account_utils.IDP_COGNITO] = account_utils.convert_user_from_cognito_to_model(user)
        except:
            print 'Failed to lookup username {} for account {}:'.format(account['CognitoUsername'], account['AccountId'])
            traceback.print_exc()
            # Skip user lookup and continue with possibly incomplete results.

def populate_account_for_user(user):
    username = user.get('Username', '')

    matching_accounts = get_accounts_by_username(username)
    if matching_accounts:
        if len(matching_accounts) > 1:
            print 'Warning: found multiple accounts associated with username "{}"'.format(username)
        account = matching_accounts[0]
        if 'IdentityProviders' not in account:
            account['IdentityProviders'] = {}
        account['IdentityProviders'][account_utils.IDP_COGNITO] = account_utils.convert_user_from_cognito_to_model(user)
        return account

    account = {
        'CognitoUsername': username,
        'IdentityProviders': {
            account_utils.IDP_COGNITO: account_utils.convert_user_from_cognito_to_model(user)
        }
    }
    return account
