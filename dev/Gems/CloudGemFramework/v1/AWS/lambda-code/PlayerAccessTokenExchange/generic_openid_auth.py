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
import httplib
import json

from auth_token_exception import AuthTokenException

def get_openid_refresh_token(auth_code, auth_settings):
      connection = httplib.HTTPSConnection(auth_settings['provider_uri'], auth_settings['provider_port']) 
      headers = { 'Content-Type': 'application/x-www-form-urlencoded;charset=UTF-8' }
      body = 'grant_type=authorization_code&code={0}&client_id={1}&client_secret={2}&redirect_uri={3}'.format(auth_code,  auth_settings['client_id'],
                              auth_settings['client_secret'],  auth_settings['redirect_uri'])
      connection.request('POST',  auth_settings['provider_path'], body, headers) 
      response = connection.getresponse()

      if response.status == 200: 
          response_data = json.loads(response.read())  
      
          openid_refresh_token_data = {
            'access_token' : response_data['access_token'],
            'expires_in' : response_data['expires_in'],
            'refresh_token' : response_data['refresh_token'],
            'id_token' : response_data['id_token']
          }

          return openid_refresh_token_data  
      else:
        raise Exception(response.read())      
      
def openid_refresh_access_token(refresh_token, auth_settings):
      connection = httplib.HTTPSConnection(auth_settings['provider_uri'], auth_settings['provider_port']) 
      headers = { 'Content-Type': 'application/x-www-form-urlencoded;charset=UTF-8' }
      body = 'grant_type=refresh_token&refresh_token={0}&client_id={1}&client_secret={2}'.format(refresh_token, auth_settings['client_id'], auth_settings['client_secret'])
      connection.request('POST', auth_settings['provider_path'], body, headers) 
      response = connection.getresponse()
    
      if response.status == 200: 
          response_data = json.loads(response.read())  
      
          openid_refresh_token_data = {
            'access_token' : response_data['access_token'],
            'expires_in' : response_data['expires_in'],
            'refresh_token' : refresh_token
          }

          return openid_refresh_token_data
      else:
        raise Exception(response.read())      
      
       
def handler(event, context, auth_settings):
    try:
       provider_name = 'open_id-' + event['auth_provider']
       if 'code' in event:
            code = event['code']
            return get_openid_refresh_token(code, auth_settings[provider_name])
       elif 'refresh_token' in event:
            refresh_token = event['refresh_token']
            return openid_refresh_access_token(refresh_token, auth_settings[provider_name])
    except Exception as e:
        raise AuthTokenException(event, context, e.message)

    raise AuthTokenException(event, context, 'code or refresh_token must be supplied as input')
