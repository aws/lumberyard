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
# $Revision: #5 $

from cgf_utils import custom_resource_response
from cgf_utils import aws_utils
from cgf_utils import properties
from resource_manager_common import stack_info
from resource_types.cognito import user_pool
from resource_types.cognito import identity_pool
import json

def handler(event, context):
    props = properties.load(event, {
            'ClientApps': properties.StringOrListOfString(),
            'ExplicitAuthFlows': properties.StringOrListOfString(default=[]),
            'RefreshTokenValidity': properties.String('30'),
            'ConfigurationKey': properties.String(), ##this is only here to force the resource handler to execute on each update to the deployment
            'LambdaConfig': properties.Dictionary({}),
            'PoolName': properties.String(),
            'Groups': properties.ObjectOrListOfObject(default=[],
                                                      schema={
                                                          'Name': properties.String(),
                                                          'Description': properties.String(''),
                                                          'Role': properties.String(),
                                                          'Precedence': properties.String('99')
                                                      }),
            'AllowAdminCreateUserOnly': properties.String('')
        })

    #give the identity pool a unique name per stack
    stack_manager = stack_info.StackInfoManager()
    stack_name = aws_utils.get_stack_name_from_stack_arn(event['StackId'])
    stack_name = stack_name.replace('-', ' ') # Prepare stack_name to be used by _associate_user_pool_with_player_access
    pool_name = props.PoolName.replace('-', ' ')
    pool_name = stack_name+pool_name
    cognito_idp_client = user_pool.get_idp_client()
    pool_id = event.get('PhysicalResourceId')
    found_pool = user_pool.get_user_pool(pool_id)

    request_type = event['RequestType']

    if request_type == 'Delete':
        if found_pool != None:
            cognito_idp_client.delete_user_pool(UserPoolId = pool_id) 
        data = {}

    else:
        #if the pool exists just update it, otherwise create a new one
        
        mfaConfig = 'OFF'   # MFA is currently unsupported by Lumberyard
        # Users are automatically prompted to verify these things. 
        # At least one auto-verified thing (email or phone) is required to allow password recovery.
        auto_verified_attributes = [ 'email' ]
        
        client_app_data = {};
        lambda_config = props.LambdaConfig

        user_pool.validate_identity_metadata(stack_manager, event['StackId'], event['LogicalResourceId'], props.ClientApps)
        admin_create_user_config = __get_admin_create_user_config(props.AllowAdminCreateUserOnly)
        print json.dumps(admin_create_user_config)

        if found_pool != None:  # Update
            response = cognito_idp_client.update_user_pool(
                    UserPoolId=pool_id,
                    MfaConfiguration=mfaConfig,
                    AutoVerifiedAttributes=auto_verified_attributes,
                    LambdaConfig=lambda_config,
                    AdminCreateUserConfig=admin_create_user_config
                )

            existing_client_apps = user_pool.get_client_apps(pool_id)
            client_app_data = update_client_apps(pool_id, props.ClientApps, existing_client_apps, False, props.ExplicitAuthFlows, props.RefreshTokenValidity)

            response = cognito_idp_client.list_groups(
                UserPoolId=pool_id
            )

            found_groups = {}
            for actual_group in response['Groups']:
                group_name = actual_group['GroupName']
                for requested_group in props.Groups:
                    # does the group exist in the resource template
                    if group_name == requested_group.Name:
                        found_groups.update({group_name: True})
                        break


                #delete the group as it is no longer in the resource template
                if group_name not in found_groups:
                    cognito_idp_client.delete_group(
                        GroupName=actual_group['GroupName'],
                        UserPoolId=pool_id
                    )

            print "Found groups=>", json.dumps(found_groups)
            #iterate the groups defined in the user pool resource template
            for group in props.Groups:
                #update the group as it is currently a group in the user pool
                group_definition = __generate_group_defintion(pool_id, group)
                print "Group '{}' is defined by {}".format(group.Name, json.dumps(group_definition))
                if group.Name in found_groups:
                    cognito_idp_client.update_group(**group_definition)
                else:
                    #group is a new group on the user pool
                    cognito_idp_client.create_group(**group_definition)

        else: # Create
            response = cognito_idp_client.create_user_pool(
                    PoolName=pool_name,
                    MfaConfiguration=mfaConfig,
                    AutoVerifiedAttributes=auto_verified_attributes,
                    LambdaConfig=lambda_config,
                    AdminCreateUserConfig=admin_create_user_config
                )
            pool_id = response['UserPool']['Id']
            print 'User pool creation response: ', response
            for group in props.Groups:
                group_definition = __generate_group_defintion(pool_id, group)
                print "Group '{}' is defined by {}".format(group.Name, json.dumps(group_definition))
                cognito_idp_client.create_group(**group_definition)

            client_app_data = update_client_apps(pool_id, props.ClientApps, [], False, props.ExplicitAuthFlows, props.RefreshTokenValidity)

        updated_resources = {
            event['StackId']: {
                event['LogicalResourceId']: {
                    'physical_id': pool_id,
                    'client_apps': {client_app['ClientName']: {'client_id': client_app['ClientId']} for client_app in client_app_data['Created'] + client_app_data['Updated']}
                }
            }
        }

        identity_pool.update_cognito_identity_providers(stack_manager, event['StackId'], pool_id, updated_resources)

        data = {
            'UserPoolName': pool_name,
            'UserPoolId': pool_id,
            'ClientApps': client_app_data,
        }
    
    physical_resource_id = pool_id

    return custom_resource_response.success_response(data, physical_resource_id)

def update_client_apps(user_pool_id, new_client_name_list, existing_clients, should_generate_secret, explicitauthflows, refreshtokenvalidity):
    client_changes = {'Created':[], 'Deleted':[], 'Updated':[]}

    new_client_names = set(new_client_name_list)

    # Update or delete existing clients.
    for existing_client in existing_clients:
        client_id = existing_client['ClientId']
        client_name = existing_client['ClientName']

        if client_name in new_client_names:
            updated = user_pool.get_idp_client().update_user_pool_client(
                UserPoolId=user_pool_id,
                ClientId=client_id,
                ExplicitAuthFlows=explicitauthflows,
                RefreshTokenValidity=int(refreshtokenvalidity)
            )
            client_changes['Updated'].append({
                'UserPoolId': user_pool_id,
                'ClientName': updated['UserPoolClient']['ClientName'],
                'ClientId': client_id,
                'ExplicitAuthFlows': explicitauthflows,
                'RefreshTokenValidity': int(refreshtokenvalidity)
            })
        else:
            deleted = user_pool.get_idp_client().delete_user_pool_client(UserPoolId=user_pool_id, ClientId=client_id)
            client_changes['Deleted'].append({
                'UserPoolId': user_pool_id,
                'ClientName': client_name,
                'ClientId': client_id
            })

    # Create new clients
    existing_client_names = {client['ClientName'] for client in existing_clients}
    for client_name in new_client_names.difference(existing_client_names):
        created = user_pool.get_idp_client().create_user_pool_client(
            UserPoolId=user_pool_id,
            ClientName=client_name,
            GenerateSecret=should_generate_secret,
            ExplicitAuthFlows=explicitauthflows,
            RefreshTokenValidity=int(refreshtokenvalidity)
        )

        client_data = {
            'UserPoolId': user_pool_id,
            'ClientName': created['UserPoolClient']['ClientName'],
            'ClientId': created['UserPoolClient']['ClientId'],
            'ExplicitAuthFlows': explicitauthflows,
            'RefreshTokenValidity': int(refreshtokenvalidity)
        }
        if 'ClientSecret' in created['UserPoolClient']:
            client_data['ClientSecret'] = created['UserPoolClient']['ClientSecret']
        client_changes['Created'].append(client_data)

    return client_changes

def __generate_group_defintion(user_pool_id, props):
    return {
        'GroupName': props.Name,
        'UserPoolId': user_pool_id,
        'RoleArn': props.Role,
        'Description': props.Description,
        'Precedence': int(props.Precedence)
    }

def __get_admin_create_user_config(allow_admin_create_user_only):
    admin_create_user_only = allow_admin_create_user_only.lower() == 'true'
    return {
        'AllowAdminCreateUserOnly': admin_create_user_only
    } if allow_admin_create_user_only else {}