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
import json
from resource_manager_common import stack_info
import user_pool

# Fields returned from describe_identity_pool that are also accepted as-is by update_identity_pool.
IDENTITY_POOL_FIELDS = ['IdentityPoolId', 'IdentityPoolName', 'AllowUnauthenticatedIdentities', 'SupportedLoginProviders', 'DeveloperProviderName', 'OpenIdConnectProviderARNs', 'CognitoIdentityProviders', 'SamlProviderARNs']

def get_identity_client():
    if not hasattr(get_identity_client, 'identity_client'):
        get_identity_client.identity_client = boto3.client('cognito-identity')
    return get_identity_client.identity_client

# Returns the identity pool if one was found, or None if the identity_pool_id is missing or invalid.
def get_identity_pool(identity_pool_id):
    if not identity_pool_id or identity_pool_id.find(':') < 0:
        # The ID is missing or invalid.
        return None
    return get_identity_client().describe_identity_pool(IdentityPoolId=identity_pool_id)

# Gets the Cognito identity providers mapped to an identity pool.
def get_cognito_identity_providers(stack_manager, stack_arn, identity_pool_logical_id):
    mappings = get_identity_mappings(stack_manager, stack_arn)
    for mapping in mappings:
        pool = mapping['identity_pool_resource']
        if pool.stack.stack_arn == stack_arn and pool.logical_id == identity_pool_logical_id:
            print 'Cognito identity providers for {}: {}'.format(identity_pool_logical_id,  mapping['providers'])
            return mapping['providers']
    print 'No Cognito identity providers for {}'
    return []

# This is called when a user pool is being updated, and identity pools need to be updated to match the new mappings from the template metadata.
# Identity pools will be affected as follows:
#   - Identity pools will be linked to the user pool if they aren't already.
#   - Identity pools that are already linked will be updated with the user pool's current client ids.
#   - Identity pools that are linked but no longer part of the mapping will be unlinked from the user pool.
def update_cognito_identity_providers(stack_manager, stack_arn, user_pool_id, updated_resources={}):
    provider_to_update =  user_pool.get_provider_name(user_pool_id)
    mappings = get_identity_mappings(stack_manager, stack_arn, updated_resources)
    
    for mapping in mappings:
        identity_pool_id = mapping['identity_pool_resource'].physical_id
        if identity_pool_id:
            identity_pool = get_identity_client().describe_identity_pool(IdentityPoolId=identity_pool_id)
            is_linked = bool([provider for provider in identity_pool.get('CognitoIdentityProviders', []) if provider.get('ProviderName') == provider_to_update])
            should_be_linked = bool([provider for provider in mapping['providers'] if provider.get('ProviderName') == provider_to_update])
            # If is_linked, the link either needs to be updated or removed.
            # If should_be_linked, the link either needs to be created or updated.
            if is_linked or should_be_linked:
                # Create an update request based on the current pool description.
                update_request = {}
                for field in IDENTITY_POOL_FIELDS:
                    if field in identity_pool:
                        update_request[field] = identity_pool[field]

                # Replace the current list of providers.  This handles linking, updating, and unlinking pools.
                update_request['CognitoIdentityProviders'] = mapping['providers']

                # Update the pool.
                get_identity_client().update_identity_pool(**update_request)

# Gets identity mappings from Custom::CognitoIdentityPool and Custom::CognitoUserPool resource metadata.
#
# The updated_resources should be provided to avoid using stale data if user pools are currently being modified.
# This method uses stack descriptions which are only updated after the custom handler completes.
#   updated_resources = {
#       '<stack-arn>' = {
#           '<user-pool-logical-id>' = {
#               'physical_id' = '<user-pool-id>',
#               'client_apps' = {
#                   '<client-app-name>' = {
#                       'client_id' = '<client-id>'
#                   }
#               }
#           }
#       }
#   }
#
# Returned structure:
#   [
#       {
#           'identity_pool_resource': instance of stack_info.ResourceInfo,
#           'providers': [ # A list to be used for the CognitoIdentityProviders parameter when creating or updating the identity pool.
#               {
#                   'ClientId': '<client-id>',
#                   'ProviderName': 'cognito-idp.<region>.amazonaws.com/<user-pool-id>',
#                   'ServerSideTokenCheck': True
#               }
#           ]
#       }
#   ]
def get_identity_mappings(stack_manager, stack_arn, updated_resources={}):
    # Collect a list of stacks to search for resource metadata.
    stacks_to_search = []
    stack = stack_manager.get_stack_info(stack_arn)
    if stack.stack_type == stack_info.StackInfo.STACK_TYPE_DEPLOYMENT_ACCESS or stack.stack_type == stack_info.StackInfo.STACK_TYPE_RESOURCE_GROUP:
        # Pools can be linked between resource groups and the deployment access stack.
        stacks_to_search.extend(stack.deployment.resource_groups)
        if stack.deployment.deployment_access:
            stacks_to_search.append(stack.deployment.deployment_access)
    elif stack.stack_type == stack.STACK_TYPE_PROJECT:
        # The project stack can have pools that aren't linked to a deployment.
        stacks_to_search.append(stack)

    # Fetch the stack descriptions and collect information from Custom::CognitoIdentityPool and Custom::CognitoUserPool resources.
    identity_pool_mappings = []
    idp_by_pool_name = {}
    for stack in stacks_to_search:
        for resource in stack.resources:
            if resource.type == 'Custom::CognitoIdentityPool':
                identity_pool_mappings.append({
                    'identity_pool_resource': resource,
                })
                print 'Found CognitoIdentityPool {}.{}'.format(stack.stack_name, resource.logical_id)
            elif resource.type == 'Custom::CognitoUserPool':
                identities = resource.metadata.get('CloudCanvas', {}).get('Identities', [])
                updated_resource = updated_resources.get(stack.stack_arn, {}).get(resource.logical_id, {})
                physical_id = updated_resource.get('physical_id', resource.physical_id)
                # Skip user pools that haven't been created yet, they will be linked later on creation.
                if physical_id:
                    for identity in identities:
                        pool_name = identity.get('IdentityPoolLogicalName')
                        client_app = identity.get('ClientApp')
                        if not client_app:
                            raise RuntimeError('Missing ClientApp in Identities metadata for stack {} resource {}'.format(
                                stack.stack_name, resource.logical_id))

                        # Get the client id from the updated_resources parameter if possible, otherwise get it from the pool.
                        client_id = updated_resource.get('client_apps', {}).get('client_app', {}).get('client_id')
                        if not client_id:
                            client_id = user_pool.get_client_id(physical_id, client_app)

                        if not client_id:
                            # A client may be missing if there's more than one pool updating at the same time and the list of clients is changing.
                            print 'Unable to find client named {} in user pool with physical id {} defined in stack {} resource {}'.format(
                                client_app, physical_id, stack.stack_name, resource.logical_id)
                        else:
                            pools = idp_by_pool_name.get(pool_name, [])
                            pools.append({
                                'ClientId': client_id,
                                'ProviderName': user_pool.get_provider_name(physical_id),
                                'ServerSideTokenCheck': True
                            })
                            idp_by_pool_name[pool_name] = pools
                            print 'Found CognitoUserPool {}.{} mapped to {} with client id {}'.format(stack.stack_name, resource.logical_id, pool_name, client_id)

    # Combine the user pool mappings with the identity pool mappings.
    for mapping in identity_pool_mappings:
        mapping['providers'] = idp_by_pool_name.get(mapping['identity_pool_resource'].logical_id, [])

    return identity_pool_mappings
