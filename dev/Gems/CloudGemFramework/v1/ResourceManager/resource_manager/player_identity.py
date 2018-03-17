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

import aws
import botocore
import json 
from errors import HandledError
from resource_manager_common import constant

key = 'player-access/'+constant.AUTH_SETTINGS_FILENAME
amazon = 'amazon'
amazon_uri = 'www.amazon.com'
google = 'google'
google_uri = 'accounts.google.com'
facebook = 'facebook'
facebook_uri = 'graph.facebook.com'
standard_https_port = 443

standard_auth_provider_uris = {
    amazon: amazon_uri,
    google: google_uri,
    facebook: facebook_uri
}

def add_login_provider(context, args):
    ##download the file from s3   
    auth_doc = _load_doc_from_s3(context)
           
    ##parse the document into json
    auth_obj = json.loads(auth_doc)

    ##add the provider info to the json doc, overwrites the existing configuration if already there.
    provider_uri = ''
    provider_name = args.provider_name

    provider_args = {}    

    if  provider_name in standard_auth_provider_uris.keys():
         provider_uri = standard_auth_provider_uris[provider_name]    
    else:
        provider_name = 'open_id-' + provider_name
        if args.provider_uri == None:
            raise HandledError('If a generic open id provider is used, a provider URI must be specified.')

        provider_uri = args.provider_uri

        if args.provider_port != None:
            provider_args['provider_port'] = args.provider_port
        else:
            provider_args['provider_port'] = standard_https_port

        if args.provider_path != None:
             provider_args['provider_path'] = args.provider_path
        else:
            raise HandledError('If a generic open id provider is used, a provider path must be specified.')

    provider_args['provider_uri'] = provider_uri 
    provider_args['app_id'] = args.app_id
    provider_args['client_id'] = args.client_id
    provider_args['client_secret'] = args.client_secret
    provider_args['redirect_uri'] = args.redirect_uri    

    auth_obj[provider_name] = provider_args
   
    ##put the document back
    _put_obj_in_s3(context, json.dumps(auth_obj))

#updates zero or more args in the provider doc
def update_login_provider(context, args):
    ##download the file from s3   
    auth_doc = _load_doc_from_s3(context)
           
    ##parse the document into json
    auth_obj = json.loads(auth_doc)

    provider_name = args.provider_name

    ##update the provider info to the json doc, or update the provider info if already there
    if provider_name not in standard_auth_provider_uris.keys():
        provider_name = 'open_id-' + provider_name

    if provider_name not in auth_obj:
        raise HandledError('Update failed: the provider "{}" does not exist.'.format(args.provider_name))

    provider_args = auth_obj[provider_name]

    if args.provider_port != None:
        provider_args['provider_port'] = args.provider_port
    
    if args.provider_path != None:
        provider_args['provider_path'] = args.provider_path
    
    if args.provider_uri != None:
        provider_args['provider_uri'] = args.provider_uri 

    if args.app_id != None:
        provider_args['app_id'] = args.app_id

    if args.client_id != None:
        provider_args['client_id'] = args.client_id

    if args.client_secret != None:
        provider_args['client_secret'] = args.client_secret

    if args.redirect_uri != None:
        provider_args['redirect_uri'] = args.redirect_uri    

    auth_obj[provider_name] = provider_args
   
    ##put the document back
    _put_obj_in_s3(context, json.dumps(auth_obj))
  
def remove_login_provider(context, args):
    auth_doc = _load_doc_from_s3(context)
           
    ##parse the document into json
    auth_obj = json.loads(auth_doc)

    ##remove the provider info from the json doc
    provider_name = args.provider_name
    if provider_name not in standard_auth_provider_uris.keys():
        provider_name = 'open_id-' + provider_name

    if provider_name in auth_obj:
        del auth_obj[provider_name]

    ##put the document back
    _put_obj_in_s3(context, json.dumps(auth_obj))
     
def _get_bucket(context):
   return context.stack.get_physical_resource_id(context.config.project_stack_id, 'Configuration')

def _load_doc_from_s3(context):
    s3_client = context.aws.client('s3')
    bucket = _get_bucket(context)
    
    auth_doc = None

    try:        
        context.view.downloading_content(bucket, key)
        auth_res = s3_client.get_object(Bucket = bucket, Key = key)
        auth_doc = auth_res['Body'].read()
    except botocore.exceptions.ClientError as e:
        error_code = e.response['Error']['Code']
        if error_code == 'NoSuchKey':
            auth_doc = '{ }'
    except Exception as e:
        raise HandledError('Error connecting to s3. {}'.format(e.message))
    
    return auth_doc

def _put_obj_in_s3(context, str):
    s3_client = context.aws.client('s3')
    bucket = _get_bucket(context)
 
    try:
        context.view.uploading_content(bucket, key, 'authentication document')
        s3_client.put_object(Bucket = bucket, Key = key, Body = str)
    except Exception as e:
       raise HandledError('Error uploading auth document to s3. {}'.format(e.message))
    
