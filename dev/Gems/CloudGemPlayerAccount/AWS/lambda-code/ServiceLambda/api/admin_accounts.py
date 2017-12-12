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
import botocore
import CloudCanvas
import errors
from random import randint
import service
import uuid

@service.api(logging_filter=account_utils.apply_logging_filter)
def get(request, AccountId):
    response = account_utils.get_account_table().get_item(Key = { 'AccountId': AccountId }, ConsistentRead = False)

    if not 'Item' in response:
        raise errors.ClientError("No account found for '{}'".format(AccountId))

    account = account_utils.convert_account_from_dynamo_to_admin_model(response['Item'])
    load_user(account)

    return account

@service.api(logging_filter=account_utils.apply_logging_filter)
def put(request, AccountId, AccountRequest):
    return put_account(AccountId, AccountRequest, create_account=False)

@service.api(logging_filter=account_utils.apply_logging_filter)
def post(request, AccountRequest):
    account_id = str(uuid.uuid4())
    return put_account(account_id, AccountRequest, create_account=True)

def put_account(AccountId, AccountRequest, create_account):
    account_utils.validate_account_update_request(AccountRequest)

    # Collect the attribute changes for the Cognito user.
    cognito_request = AccountRequest.get('IdentityProviders', {}).get(account_utils.IDP_COGNITO, {})
    cognito_updates = get_cognito_updates(cognito_request)

    # Get the existing account if needed and determine the Cognito username.
    create_user = False
    existing_account = {}
    username = None
    if create_account:
        username = AccountRequest.get('CognitoUsername')
        create_user = 'CognitoUsername' in AccountRequest
    else:
        existing_account = account_utils.get_account_table().get_item(Key = { 'AccountId': AccountId }, ConsistentRead = False).get('Item', {})
        username = existing_account.get('CognitoUsername')
        if 'CognitoUsername' in AccountRequest and 'CognitoUsername' in existing_account and username != AccountRequest.get('CognitoUsername'):
            raise errors.ClientError('Changing the account username is not supported.')
        create_user = 'CognitoUsername' in AccountRequest and 'CognitoUsername' not in existing_account

    if cognito_updates and not username:
        raise errors.ClientError('Unable to update Cognito: There is no Cognito user associated with this account.')

    # Collect the attribute changes for the account row.
    account_updates, delete_keys, added_to_blacklist = get_account_updates(AccountRequest, existing_account)

    # Create or update the account.
    updated_account = None
    account = account_updates.copy()
    account['AccountId'] = AccountId
    if create_account:
        account_utils.create_account(account)
        updated_account = account
        print 'Created account: ', account
    elif account_updates or delete_keys:
        updated_account = account_utils.update_account(account, delete_keys, existing_account)
        print 'Updated account: ', account

    # Create or update the Cognito user, and roll back the account changes if that fails.
    try:
        if create_user:
            account_utils.get_user_pool_client().admin_create_user(
                UserPoolId=account_utils.get_user_pool_id(),
                Username=username,
                UserAttributes=cognito_updates,
                DesiredDeliveryMediums=['EMAIL']
            )
            print 'Created: ', account_utils.logging_filter(cognito_updates)
        elif cognito_updates:
            account_utils.get_user_pool_client().admin_update_user_attributes(
                UserPoolId=account_utils.get_user_pool_id(),
                Username=username,
                UserAttributes=cognito_updates
            )
            print 'Updated: ', account_utils.logging_filter(cognito_updates)
    except botocore.exceptions.ClientError as e:
        if updated_account:
            undo_account_changes(AccountId, create_account, existing_account, account_updates)

        code = e.response.get('Error', {}).get('Code', None)
        if code == 'UserNotFoundException':
            raise errors.ClientError('User does not exist in Cognito.')
        if code == 'UsernameExistsException':
            raise errors.ClientError('The username already exists in Cognito.')
        raise
    except:
        if updated_account:
            undo_account_changes(AccountId, create_account, existing_account, account_updates)

        raise

    if added_to_blacklist and username:
        # Invalidate existing tokens for the user after adding to the blacklist.
        account_utils.get_user_pool_client().admin_user_global_sign_out(UserPoolId=account_utils.get_user_pool_id(), Username=username)

    account = account_utils.convert_account_from_dynamo_to_admin_model(updated_account or existing_account)
    load_user(account)

    return account

def load_user(account):
    if 'CognitoUsername' in account:
        try:
            user = account_utils.get_user_pool_client().admin_get_user(
                UserPoolId=account_utils.get_user_pool_id(),
                Username=account['CognitoUsername']
            )
            account['IdentityProviders'] = {}
            account['IdentityProviders'][account_utils.IDP_COGNITO] = account_utils.convert_user_from_cognito_to_model(user)
        except botocore.exceptions.ClientError as e:
            code = e.response.get('Error', {}).get('Code', None)
            if code == 'UserNotFoundException':
                print 'User {} not found for account {}.'.format(account['CognitoUsername'], account.get('AccountId'))
                return
            raise

def undo_account_changes(AccountId, account_was_created, existingAccount, accountUpdates):
    if account_was_created:
        response = account_utils.get_account_table().delete_item(Key={'AccountId': AccountId}, ReturnValues='ALL_OLD')
        print 'Rolled back account creation for {}'.format(response.get('Attributes'))
    else:
        delete_keys = set()
        account_rollback = {}

        for k,v in accountUpdates.iteritems():
            if k in existingAccount:
                account_rollback[k] = existingAccount[k]
            else:
                delete_keys.add(k)

        account_rollback['AccountId'] = AccountId
        account_utils.update_account(account_rollback, delete_keys)
        print 'Rolled back account changes.'

# Determine the changes needed to make an existing account match the request.
def get_account_updates(account_request, existing_account):
    account_updates = {}
    delete_keys = set()
    if 'PlayerName' in account_request and existing_account.get('PlayerName') != account_request.get('PlayerName'):
        # IndexedPlayerName and PlayerNameSortKey are used to search accounts by name, case insensitive.  See accountsearch.py
        account_updates['PlayerName'] = account_request['PlayerName']
        account_updates['IndexedPlayerName'] = account_request['PlayerName'].lower()
        account_updates['PlayerNameSortKey'] = randint(1, account_utils.get_name_sort_key_count())

    request_account_blacklisted = bool(account_request.get('AccountBlacklisted'))
    added_to_blacklist = False
    if 'AccountBlacklisted' in account_request and bool(existing_account.get('AccountBlacklisted')) != request_account_blacklisted:
        if request_account_blacklisted:
            account_updates['AccountBlacklisted'] = True
            added_to_blacklist = True
        else:
            delete_keys.add('AccountBlacklisted')

    if 'CognitoUsername' in account_request and existing_account.get('CognitoUsername') != account_request.get('CognitoUsername'):
        account_updates['CognitoUsername'] = account_request['CognitoUsername']

    return account_updates, delete_keys, added_to_blacklist

# Convert the Cognito user request into the format needed by the Cognito client.
def get_cognito_updates(cognito_request):
    cognito_updates = []
    for key, value in cognito_request.iteritems():
        if key in account_utils.COGNITO_ATTRIBUTES:
            cognito_updates.append({'Name': key, 'Value': value})
        else:
            raise errors.ClientError('Invalid field {}.'.format(key))
    return cognito_updates
