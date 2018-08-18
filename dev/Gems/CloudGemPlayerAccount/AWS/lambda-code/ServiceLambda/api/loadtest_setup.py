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
 
from __future__ import print_function
 
import service
 
import admin_accounts
import admin_accountSearch
 
GEM_NAME = 'CloudGemLoadTest'
PLAYER_COUNT = 4  # Using more than 4 may run into AWS Cognito throttling failures


#
# Cloud Gem Player Account Load Test Setup Initialization API
#
@service.api
def post(request):
    print("Player Account load test initialization entered.")
    
    out = {'accounts':[]}
    num_active_accounts = get_num_active_accounts()
    
    if(num_active_accounts < PLAYER_COUNT):
        out = create_load_test_users(request, out)
    else:
        print("Enough accounts found to perform Player Account load tests.")   
    
    print("Player Account load test initialization has finished.")
    return out


#
# Get the number of active accounts for the Player Account DB via the admin API
#
def get_num_active_accounts():
    account_list_response = admin_accountSearch.default_search()
    account_list = account_list_response['Accounts']

    return len(account_list)


# 
# Create player account load test users flow
#
def create_load_test_users(request, out):
    print('Not enough accounts found. Adding {} test accounts to {}.'.format(str(PLAYER_COUNT), GEM_NAME))

    for x in range(0, PLAYER_COUNT):
        account = generate_load_test_account_info(x)
        out = add_load_test_user(request, account, out)

    return out


#
# Generate info for player account load test user
#
def generate_load_test_account_info(count):
    account_suffix = str(count)

    account = {
            "AccountBlacklisted": True,
            'CognitoUsername': 'LoadTestAccount' + account_suffix,
            'PlayerName': "LoadTest" + account_suffix,
            'IdentityProviders': {
                'Cognito': {
                    'email': 'LoadTestEmail' + account_suffix + '@test.test',
                    'given_name': 'Load' + account_suffix,
                    'family_name': 'Test',
                    'nickname': "TestLoad" + account_suffix,
                }
            }
        }

    return account


#
# Add/Post the load test user to the Player Account DB
#
def add_load_test_user(request, account, out):
    try:
        result = admin_accounts.post(request, account)
        out['accounts'].append(result['AccountId'])
    except Warning:
        print ('Failed to create account {}, likely due to duplicate CognitoUserNames'
               .format(account['CognitoUsername']))

    return out
