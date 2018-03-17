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
# $Revision: #1 $

from cgf_utils import properties
from cgf_utils import custom_resource_response
import boto3
import botocore
import json
from cgf_utils import aws_utils
from resource_manager_common import constant
from resource_types.cognito import identity_pool
from resource_manager_common import stack_info

def handler(event, context):

    props = properties.load(event, {
            'ConfigurationBucket': properties.String(),
            'ConfigurationKey': properties.String(), ##this is only here to force the resource handler to execute on each update to the deployment
            'IdentityPoolName': properties.String(),
            'UseAuthSettingsObject': properties.String(),
            'AllowUnauthenticatedIdentities': properties.String(),
            'DeveloperProviderName': properties.String(default=''),
            'Roles': properties.Object( default={}, 
            schema={
                '*': properties.String()
            }),
            'RoleMappings': properties.Object(default={},
                schema={
                    'Cognito': properties.Object(default={}, schema={
                        'Type': properties.String(''),
                        'AmbiguousRoleResolution': properties.String('')
                    })
                }
            )
        })

    #give the identity pool a unique name per stack
    stack_manager = stack_info.StackInfoManager()
    stack_name = aws_utils.get_stack_name_from_stack_arn(event['StackId'])
    identity_pool_name = stack_name+props.IdentityPoolName
    identity_pool_name = identity_pool_name.replace('-', ' ')
    identity_client = identity_pool.get_identity_client()
    identity_pool_id = event.get('PhysicalResourceId')
    found_pool = identity_pool.get_identity_pool(identity_pool_id)

    request_type = event['RequestType']
    if request_type == 'Delete':
        if found_pool != None:
            identity_client.delete_identity_pool(IdentityPoolId=identity_pool_id) 
        data = {}       

    else:
        use_auth_settings_object = props.UseAuthSettingsObject.lower() == 'true'
        supported_login_providers = {}

        if use_auth_settings_object == True:
            #download the auth settings from s3
            player_access_key = 'player-access/'+constant.AUTH_SETTINGS_FILENAME
            auth_doc = json.loads(_load_doc_from_s3(props.ConfigurationBucket, player_access_key))             

            #if the doc has entries add them to the supported_login_providers dictionary
            if len(auth_doc) > 0:
                for key, value in auth_doc.iteritems():
                    supported_login_providers[value['provider_uri']] = value['app_id']         

        cognito_identity_providers = identity_pool.get_cognito_identity_providers(stack_manager, event['StackId'], event['LogicalResourceId'])

        print 'Identity Providers: ', cognito_identity_providers
        allow_anonymous = props.AllowUnauthenticatedIdentities.lower() == 'true'
        #if the pool exists just update it, otherwise create a new one
        
        args = {
            'IdentityPoolName': identity_pool_name, 
            'AllowUnauthenticatedIdentities': allow_anonymous,
            'SupportedLoginProviders': supported_login_providers, 
            'CognitoIdentityProviders': cognito_identity_providers
        }
        
        if props.DeveloperProviderName:
            args['DeveloperProviderName'] = props.DeveloperProviderName
        
        if found_pool != None:
           identity_client.update_identity_pool(IdentityPoolId=identity_pool_id, **args)    
        else:
           response = identity_client.create_identity_pool(**args) 
           identity_pool_id=response['IdentityPoolId'] 

        #update the roles for the pool
        role_mappings = {}
        if props.RoleMappings.Cognito.Type and len(cognito_identity_providers) > 0:
            print 'Adding role mappings for cognito', props.RoleMappings.Cognito.__dict__
            role_mappings['{}:{}'.format(cognito_identity_providers[0]['ProviderName'],cognito_identity_providers[0]['ClientId'])]=props.RoleMappings.Cognito.__dict__

        print "Role Mappings: ", role_mappings
        identity_client.set_identity_pool_roles(
            IdentityPoolId=identity_pool_id,
            Roles=props.Roles.__dict__,
            RoleMappings=role_mappings)

        data = {
                'IdentityPoolName': identity_pool_name,
                'IdentityPoolId': identity_pool_id       
        }  
    
    physical_resource_id = identity_pool_id

    return custom_resource_response.success_response(data, physical_resource_id)

def _load_doc_from_s3(bucket, key):
    s3_client = boto3.client('s3', region_name=aws_utils.current_region)    
    
    auth_doc = None

    try:        
        auth_res = s3_client.get_object(Bucket = bucket, Key = key)
        auth_doc = auth_res['Body'].read()
    except botocore.exceptions.ClientError as e:
        error_code = e.response['Error']['Code']
        if error_code == 'NoSuchKey' or error_code == 'AccessDenied':
            auth_doc = '{ }'       
    
    return auth_doc
