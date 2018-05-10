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
from boto3.dynamodb.conditions import Key
import botocore.exceptions
import CloudCanvas
from copy import deepcopy
from datetime import datetime
from dateutil import tz
import errors
import json
import types

IDP_COGNITO = 'Cognito'
COGNITO_ATTRIBUTES = {"address", "birthdate", "email", "family_name", "gender", "given_name", "locale", "middle_name", "nickname", "phone_number", "picture", "profile", "website", "zoneinfo"}

ACCOUNT_FIELDS_RETURNED_FOR_PLAYERS = {'AccountId', 'PlayerName'}
ACCOUNT_FIELDS_RETURNED_FOR_ADMINS = {'AccountId', 'AccountBlacklisted', 'PlayerName', 'CognitoIdentityId', 'CognitoUsername'}

MAX_PLAYER_NAME_LENGTH = 30
MAX_COGNITO_ATTRIBUTE_LENGTH = 2048

def get_user_pool_client():
    if not hasattr(get_user_pool_client, 'user_pool_client'):
        get_user_pool_client.user_pool_client = boto3.client('cognito-idp')
    return get_user_pool_client.user_pool_client

def get_identity_pool_client():
    if not hasattr(get_identity_pool_client, 'identity_pool_client'):
        get_identity_pool_client.identity_pool_client = boto3.client('cognito-identity')
    return get_identity_pool_client.identity_pool_client

def get_account_table():
    if not hasattr(get_account_table, 'account_table'):
        account_table_name = CloudCanvas.get_setting('PlayerAccounts')
        get_account_table.account_table = boto3.resource('dynamodb').Table(account_table_name)
        if get_account_table.account_table is None:
            raise RuntimeError('Unable to create client for the account table.')
    return get_account_table.account_table

def get_name_sort_key_count():
    sortKeyCount = int(CloudCanvas.get_setting('PlayerAccountNameSortKeyCount'))
    if sortKeyCount < 1:
        raise RuntimeError('PlayerAccountNameSortKeyCount must be at least one')
    return sortKeyCount

def get_user_pool_id():
    pool_id = CloudCanvas.get_setting('UserPool')
    if not pool_id:
        raise RuntimeError("Missing setting 'UserPool'")
    return pool_id

def get_identity_pool_id():
    identity_pool_id = CloudCanvas.get_setting('CloudCanvas::IdentityPool')
    if identity_pool_id:
        return identity_pool_id

    if not hasattr(get_identity_pool_id, 'identity_pool_id'):
        # TODO: switch this over to using service iterfaces and the service directory once available.
        # Currently we invoke a service api lambda directly.
        lambda_client = boto3.client('lambda')
        response = lambda_client.invoke(
            FunctionName = CloudCanvas.get_setting('CloudCanvas::ServiceLambda'),
            Payload = json.dumps({
                'function': 'get_deployment_access_resource_info',
                'module': 'resource_info',
                'parameters': {
                    'deployment_name': CloudCanvas.get_setting('CloudCanvas::DeploymentName'),
                    'resource_name': 'PlayerAccessIdentityPool'
                }
            })
        )
        resource_info = json.loads(response['Payload'].read())
        get_identity_pool_id.identity_pool_id = resource_info['PhysicalId']

    return get_identity_pool_id.identity_pool_id

def get_username_for_account(AccountId):
    account = get_account_table().get_item(Key = { 'AccountId': AccountId }, ConsistentRead = False)

    if not 'Item' in account:
        raise errors.ClientError("No account found for '{}'".format(AccountId))

    if not 'CognitoUsername' in account['Item']:
        raise errors.ClientError("Account {} is not linked to a Cognito user pool".format(AccountId))

    return account['Item']['CognitoUsername']

def get_account_for_identity(cognitoIdentityId):
    response = get_account_table().query(
        ConsistentRead=False,
        IndexName='CognitoIdentityIdIndex',
        KeyConditionExpression=Key('CognitoIdentityId').eq(cognitoIdentityId),
        Limit=2
    )
    items = response.get('Items', [])
    if len(items) == 1:
        return items[0]
    if len(items) > 1:
        print 'Warning: More than one account found for {}'.format(cognitoIdentityId)
        return items[0]
    return None

def get_account_for_user_name(username):
    response = get_account_table().query(
        ConsistentRead=False,
        IndexName='CognitoUsernameIndex',
        KeyConditionExpression=Key('CognitoUsername').eq(username),
        Limit=2
    )
    items = response.get('Items', [])
    if len(items) == 1:
        return items[0]
    if len(items) > 1:
        print 'Warning: More than one account found for {}'.format(username)
        return items[0]
    return None

def get_user(username):
    try:
        return get_user_pool_client().admin_get_user(
            UserPoolId=get_user_pool_id(),
            Username=username
        )
    except botocore.exceptions.ClientError as e:
        code = e.response.get('Error', {}).get('Code', None)
        if code == 'UserNotFoundException':
            raise errors.ClientError('User does not exist')
        raise

def convert_account_from_dynamo_to_player_model(item):
    return {k:v for k,v in item.iteritems() if k in ACCOUNT_FIELDS_RETURNED_FOR_PLAYERS}

def convert_account_from_dynamo_to_admin_model(item):
    return {k:v for k,v in item.iteritems() if k in ACCOUNT_FIELDS_RETURNED_FOR_ADMINS}

def convert_user_from_cognito_to_model(user, accountId=None):
    result = {
        'IdentityProviderId': IDP_COGNITO,
        'username': user['Username']
    }

    if accountId:
        result['AccountId'] = accountId

    if 'UserStatus' in user:
        result['status'] = user['UserStatus']
    if 'Enabled' in user:
        result['enabled'] = user['Enabled']
    if 'UserCreateDate' in user:
        result['create_date'] = (user['UserCreateDate'] - datetime(1970, 1, 1, tzinfo=tz.tzutc())).total_seconds()
    if 'UserLastModifiedDate' in user:
        result['last_modified_date'] = (user['UserLastModifiedDate'] - datetime(1970, 1, 1, tzinfo=tz.tzutc())).total_seconds()

    # Cognito returns Attributes for list users, and UserAttributes for get user.
    result.update({attribute['Name']: attribute['Value'] for attribute in user.get('Attributes', []) if attribute['Name'] in COGNITO_ATTRIBUTES})
    result.update({attribute['Name']: attribute['Value'] for attribute in user.get('UserAttributes', []) if attribute['Name'] in COGNITO_ATTRIBUTES})

    return result

def create_account(item):
    get_account_table().put_item(
        Item=item,
        ConditionExpression='attribute_not_exists(AccountId)'
    )
    print 'Added account {}'.format(item['AccountId'])

def update_account(item, delete_keys=set(), existing_account=None):
    # Reads are cheaper than writes, check if a write is actually needed.
    account_key = { 'AccountId': item['AccountId'] }

    if not existing_account:
        get_account_response = get_account_table().get_item(Key=account_key, ConsistentRead=False)
        print 'Existing account row: ', get_account_response

        if 'Item' not in get_account_response:
            raise errors.ClientError('Account {} does not exist.'.format(item['AccountId']))

        existing_account = get_account_response['Item']

    updates = {}
    updated_item = existing_account.copy()
    for key, value in item.iteritems():
        if key not in existing_account or existing_account[key] != value:
            updates[key] =  {'Value': value, 'Action': 'PUT'}
            updated_item[key] = value
    for key in delete_keys:
        updates[key] =  {'Action': 'DELETE'}
        del updated_item[key]

    if not updates:
        print 'The account table is up to date.'
        return existing_account

    get_account_table().update_item(
        Key=account_key,
        AttributeUpdates=updates
    )
    print 'Updated the account table: ', updates
    return updated_item

def create_or_update_account(item, deleteKeys=set()):
    account_key = { 'AccountId': item['AccountId'] }
    get_account_response = get_account_table().get_item(Key=account_key, ConsistentRead=False)
    print 'Existing account row: ', get_account_response

    if 'Item' not in get_account_response:
        # Account doesn't exist. Create a new one.
        get_account_table().put_item(
            Item=item,
            ConditionExpression='attribute_not_exists(AccountId)'
        )
        print 'Added account.'
        return item

    existing_item = get_account_response['Item']
    updates = {}
    updated_item = existing_item.copy()
    for key, value in item.iteritems():
        if key not in existing_item or existing_item[key] != value:
            updates[key] =  {'Value': value, 'Action': 'PUT'}
            updated_item[key] = value
    for key in deleteKeys:
        updates[key] =  {'Action': 'DELETE'}
        del updated_item[key]

    if not updates:
        print 'The account table is up to date.'
        return existing_item

    get_account_table().update_item(
        Key=account_key,
        AttributeUpdates=updates
    )
    print 'Updated the account table: ', updates
    return updated_item

def logging_filter(value_to_filter):
    result = deepcopy(value_to_filter)
    apply_logging_filter(result)
    return result

def apply_logging_filter(value_to_update):
    if type(value_to_update) == types.DictType:
        if 'email' in value_to_update:
            value_to_update['email'] = '<redacted>'
        if 'Email' in value_to_update:
            value_to_update['Email'] = '<redacted>'
        if value_to_update.get('Name') == 'email':
            value_to_update['Value'] = '<redacted>'
        for key,value in value_to_update.iteritems():
            apply_logging_filter(value)
    if type(value_to_update) == types.ListType:
        for value in value_to_update:
            apply_logging_filter(value)

def validate_account_update_request(account):
    if len(account.get('PlayerName', '')) > MAX_PLAYER_NAME_LENGTH:
        raise errors.ClientError('PlayerName is longer than {} characters'.format(MAX_PLAYER_NAME_LENGTH))
    for key,value in account.get('IdentityProviders', {}).get(IDP_COGNITO, {}).iteritems():
        if len(value) > MAX_COGNITO_ATTRIBUTE_LENGTH:
            raise errors.ClientError('{} is longer than {} characters'.format(key, MAX_COGNITO_ATTRIBUTE_LENGTH))

def compare_accounts(a, b):
    a_name = a.get('PlayerName', '')
    b_name = b.get('PlayerName', '')

    # Compare by name, case insensitive.
    result = cmp(a_name.lower(), b_name.lower())
    if result:
        return result

    # Tie break by name.
    result = cmp(a_name, b_name)
    if result:
        return result

    a_username = a.get('CognitoUsername', '')
    b_username = b.get('CognitoUsername', '')

    # Compare by username, case insensitive.
    result = cmp(a_username.lower(), b_username.lower())
    if result:
        return result

    # Tie break by account id.
    return cmp(a.get('AccountId', ''), b.get('AccountId', ''))
