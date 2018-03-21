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
# $Revision$

import boto3
from resource_manager_common import stack_info

def get_idp_client():
    if not hasattr(get_idp_client, 'idp_client'):
        get_idp_client.idp_client = boto3.client('cognito-idp')
    return get_idp_client.idp_client

# Returns the user pool if one was found, or None if the identity_pool_id is missing or invalid.
def get_user_pool(user_pool_id):
    if not user_pool_id or user_pool_id.find('_') < 0:
        # The ID is missing or invalid.
        return None
    return get_idp_client().describe_user_pool(UserPoolId=user_pool_id)

# Looks up a client id using the pool id and client name.
def get_client_id(user_pool_id, client_app_name):
    for client_app in get_client_apps(user_pool_id):
        if client_app.get('ClientName') == client_app_name:
            return client_app.get('ClientId')
    return None

# Gets all client apps for a pool.
def get_client_apps(user_pool_id):
    response = get_idp_client().list_user_pool_clients(UserPoolId=user_pool_id, MaxResults=60)
    client_apps = response.get('UserPoolClients', [])
    while 'NextToken' in response:
        response = get_idp_client().list_user_pool_clients(UserPoolId=user_pool_id, MaxResults=60, NextToken=response['NextToken'])
        client_apps.extend(response.get('UserPoolClients', []))
    return client_apps

# Gets the provider name for a user pool.  This is needed for creating and updating identity pools.
def get_provider_name(user_pool_id):
    # User pool IDs are of the form: us-east-1_123456789
    # Provider names are of the form: cognito-idp.us-east-1.amazonaws.com/us-east-1_123456789
    beginning = "cognito-idp."
    middle = ".amazonaws.com/"
    region_size = user_pool_id.find("_")  # Get the region from the first part of the Pool ID
    region = ""
    if region_size >= 0:
        region = user_pool_id[0 : region_size]
    
    return beginning + region + middle + user_pool_id

# A quick check to ensure that the metadata looks valid.
def validate_identity_metadata(stack_manager, stack_arn, user_pool_logical_id, client_names):
    client_names_set = set(client_names)
    stack = stack_manager.get_stack_info(stack_arn)
    user_pool = stack.resources.get_by_logical_id(user_pool_logical_id)
    for identity in user_pool.metadata.get('CloudCanvas', {}).get('Identities', []):
        if 'IdentityPoolLogicalName' not in identity:
            raise RuntimeError('Missing IdentityPoolLogicalName in Identities metadata')
        client_app = identity.get('ClientApp')
        if not client_app:
            raise RuntimeError('Missing ClientApp in Identities metadata')
        if client_app not in client_names_set:
            raise RuntimeError('ClientApp {} is not in the list of ClientApps'.format(client_app))
