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

import lwa_auth
import generic_openid_auth
import google_auth
import facebook_auth
from auth_token_exception import AuthTokenException
import discovery_utils
import boto3
import uuid
import json
import threading
import traceback

##this file needs to have the format
##{
##   authProvider {
##                   clientSpecificKeys: blah,
##                   .....
##   }
##}
auth_settings_file_path = '/tmp/{}{}'.format(uuid.uuid4(), 'auth-settings.json')
s3Config = discovery_utils.get_configuration_bucket()
bucket = s3Config.configuration_bucket
key = 'player-access/auth-settings.json'
auth_settings = None
lock = threading.Lock()

handlers = {
    'amazon': lwa_auth.handler,   
    'google': google_auth.handler,
    'facebook'  : facebook_auth.handler
}

def handler(event, context):
    global auth_settings

    try:
        auth_handler = handlers.get(event['auth_provider'], generic_openid_auth.handler)

        ##here we want to set the auth_settings object only once per lambda container.
        ##if it hasn't been set yet, download the object from s3 and set the keys object.       
        if auth_settings is None:
            print 'auth_settings object empty, attempting to aquire download lock.'
            with lock:
                print 'lock aquired'
                if auth_settings is None:
                    print 'auth_settings object still empty, attempting to download object.'
                    try:
                        s3_client = boto3.client('s3', region_name=discovery_utils.current_region)
                        print 'fetching object {}/{}'.format(bucket, key)
                        s3_client.download_file(bucket, key, auth_settings_file_path)
                        print 'object successfully downloaded to {}'.format(auth_settings_file_path)
                        with open(auth_settings_file_path) as auth_settings_file:
                            auth_settings = json.load(auth_settings_file)                            
                    except Exception as e:
                        raise AuthTokenException(event, context, str(e))
                else:
                    print 'auth_settings initialized before lock was aquired, using cached version.'
        else:
            print 'auth_settings already initialized, using cached version.'

        ##invoke the actual auth handler now
        return auth_handler(event, context, auth_settings)

    except Exception as e:
        raise AuthTokenException(event, context, 'Unexpected error occured. {}'.format(traceback.format_exc()))
    
    
