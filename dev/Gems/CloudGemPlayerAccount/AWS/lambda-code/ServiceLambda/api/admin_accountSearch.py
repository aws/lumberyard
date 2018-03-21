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
import dynamodb_pagination
import errors
import service
import traceback

@service.api(logging_filter=account_utils.apply_logging_filter)
def get(request, StartPlayerName='', CognitoIdentityId=None, CognitoUsername=None, Email=None, PageToken=None):
    if CognitoIdentityId:
        return search_by_cognito_identity_id(CognitoIdentityId)
    if CognitoUsername:
        return search_by_username(CognitoUsername)
    if Email:
        return search_by_email(Email)
    if PageToken:
        return search_by_start_name(start_player_name=StartPlayerName.lower(), serialized_page_token=PageToken)
    if StartPlayerName:
        return search_by_start_name(start_player_name=StartPlayerName.lower())

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

def search_by_start_name(start_player_name = None, serialized_page_token = None):
    page_size = 20

    config = dynamodb_pagination.PartitionedIndexConfig(
        table=account_utils.get_account_table(),
        index_name='PlayerNameIndex',
        partition_key_name='PlayerNameSortKey',
        sort_key_name='IndexedPlayerName',
        partition_count=account_utils.get_name_sort_key_count(),
        required_fields={ 'AccountId': ' ' }
    )

    if serialized_page_token:
        page_token = dynamodb_pagination.PageToken(config, serialized_page_token)
    else:
        page_token = dynamodb_pagination.get_page_token_for_inclusive_start(config, start_player_name, forward=True)

    search = dynamodb_pagination.PaginatedSearch(config, page_token, start_player_name)

    raw_account_items = search.get_next_page(page_size)
    
    accounts = [account_utils.convert_account_from_dynamo_to_admin_model(item) for item in raw_account_items]
    populate_identity_providers(accounts)

    result = {'Accounts': accounts}

    forward_token = search.get_page_token(forward=True)
    if forward_token:
        result['next'] = forward_token

    backward_token = search.get_page_token(forward=False)
    if backward_token:
        result['previous'] = backward_token

    return result

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
