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

def get_amazon_refresh_token(auth_code, auth_settings):
      connection = httplib.HTTPSConnection('api.amazon.com', 443) 
      headers = { 'Content-Type': 'application/x-www-form-urlencoded;charset=UTF-8' }
      body = 'grant_type=authorization_code&code={0}&client_id={1}&client_secret={2}&redirect_uri={3}'.format(auth_code, auth_settings['client_id'],
                             auth_settings['client_secret'],auth_settings['redirect_uri'])
      connection.request('POST', '/auth/o2/token', body, headers) 
      response = connection.getresponse()

      if response.status == 200: 
          response_data = json.loads(response.read())  
          print 'response_data {}'.format(response_data)
          amzn_refresh_token_data = {
            'access_token' : response_data['access_token'],
            'expires_in' : response_data['expires_in'],
            'refresh_token' : response_data['refresh_token']
          }

          return amzn_refresh_token_data  
      else:
        error_result = response.read()
        print 'error {}'.format(error_result)
        raise Exception(response.read())      
      
def amazon_refresh_access_token(refresh_token, auth_settings):
      connection = httplib.HTTPSConnection('api.amazon.com', 443) 
      headers = { 'Content-Type': 'application/x-www-form-urlencoded;charset=UTF-8' }
      body = 'grant_type=refresh_token&refresh_token={0}&client_id={1}&client_secret={2}&redirect_uri={3}'.format(refresh_token, auth_settings['client_id'], 
                    auth_settings['client_secret'], auth_settings['redirect_uri'])
      connection.request('POST', '/auth/o2/token', body, headers) 
      response = connection.getresponse()
    
      if response.status == 200: 
          response_data = json.loads(response.read())  
          print 'response_data {}'.format(response_data)
          amzn_refresh_token_data = {
            'access_token' : response_data['access_token'],
            'expires_in' : response_data['expires_in'],
            'refresh_token' : refresh_token
          }

          return amzn_refresh_token_data
      else:
        error_result = response.read()
        print 'error {}'.format(error_result)
        raise Exception(error_result)      
      
       
def handler(event, context, auth_settings):
    try:
       if 'code' in event:
            code = event['code']
            print 'code {}'.format(code)
            return get_amazon_refresh_token(code, auth_settings['amazon'])
       elif 'refresh_token' in event:
            refresh_token = event['refresh_token']
            print 'refresh_token {}'.format(refresh_token)
            return amazon_refresh_access_token(refresh_token, auth_settings['amazon'])
    except Exception as e:
        print e.message
        raise AuthTokenException(event, context, e.message)

    print 'error {}'.format('code or refresh_token must be supplied as input')
    raise AuthTokenException(event, context, 'code or refresh_token must be supplied as input')
