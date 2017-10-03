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

import properties
import custom_resource_response
import boto3
import botocore
import json
import discovery_utils

def handler(event, context):

    props = properties.load(event, {
            'ConfigurationBucket': properties.String(),
            'ConfigurationKey': properties.String(), ##this is only here to force the resource handler to execute on each update to the deployment
            'IdentityPoolName': properties.String(),
            'UseAuthSettingsObject': properties.String(),
            'AllowUnauthenticatedIdentities': properties.String(),            
            'Roles': properties.Object( default={}, 
            schema={
                '*': properties.String()
            }),         
        })

    #give the identity pool a unique name per stack
    stack_name = discovery_utils.get_stack_name_from_stack_arn(event['StackId'])
    identity_pool_name = props.IdentityPoolName + stack_name
    identity_pool_name = identity_pool_name.replace('-', ' ')
    cognito_client = boto3.client('cognito-identity') 
    found_pool = _find_identity_pool(cognito_client, identity_pool_name)
    identity_pool_id = None

    request_type = event['RequestType']
    if request_type == 'Delete':
        if found_pool != None:
            identity_pool_id = found_pool['IdentityPoolId']
            cognito_client.delete_identity_pool(IdentityPoolId=identity_pool_id) 
        data = {}       

    else:
        use_auth_settings_object = props.UseAuthSettingsObject.lower() == 'true'
        supported_login_providers = {}

        if use_auth_settings_object == True:
            #download the auth settings from s3
            player_access_key = 'player-access/auth-settings.json'
            auth_doc = json.loads(_load_doc_from_s3(props.ConfigurationBucket, player_access_key))             

            #if the doc has entries add them to the supported_login_providers dictionary
            if len(auth_doc) > 0:
                for key, value in auth_doc.iteritems():
                    supported_login_providers[value['provider_uri']] = value['app_id']         
        
       
        allow_anonymous = props.AllowUnauthenticatedIdentities.lower() == 'true'
        #if the pool exists just update it, otherwise create a new one
        if found_pool != None:
           response = cognito_client.update_identity_pool(IdentityPoolId=found_pool['IdentityPoolId'], IdentityPoolName=identity_pool_name, AllowUnauthenticatedIdentities=allow_anonymous,
                                                             SupportedLoginProviders=supported_login_providers)    
           identity_pool_id=found_pool['IdentityPoolId']        
                        
        else:
           response = cognito_client.create_identity_pool(IdentityPoolName = identity_pool_name, AllowUnauthenticatedIdentities=allow_anonymous,
                                                            SupportedLoginProviders=supported_login_providers) 
           identity_pool_id=response['IdentityPoolId'] 

        #now update the roles for the pool   
        cognito_client.set_identity_pool_roles(IdentityPoolId=identity_pool_id, Roles=props.Roles.__dict__)    

        data = {
                'IdentityPoolName': identity_pool_name,
                'IdentityPoolId': identity_pool_id       
        }  
    
    physical_resource_id = identity_pool_id

    custom_resource_response.succeed(event, context, data, physical_resource_id)


def _find_identity_pool(client, pool_name):
    #list the identity pools and see if the pool already exists
    identity_pools = client.list_identity_pools(MaxResults=60)
    found_pool = None
    
    if identity_pools['IdentityPools'] != None:
        for identity_pool in identity_pools['IdentityPools']:
            if identity_pool['IdentityPoolName'] == pool_name:
                return identity_pool
    
    return None

def _load_doc_from_s3(bucket, key):
    s3_client = boto3.client('s3', region_name=discovery_utils.current_region)    
    
    auth_doc = None

    try:        
        auth_res = s3_client.get_object(Bucket = bucket, Key = key)
        auth_doc = auth_res['Body'].read()
    except botocore.exceptions.ClientError as e:
        error_code = e.response['Error']['Code']
        if error_code == 'NoSuchKey' or error_code == 'AccessDenied':
            auth_doc = '{ }'       
    
    return auth_doc
