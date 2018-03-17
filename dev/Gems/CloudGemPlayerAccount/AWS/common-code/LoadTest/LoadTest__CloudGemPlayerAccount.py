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

from cloud_gem_load_test.service_api_call import ServiceApiCall
from random import randint

GEM_NAME = 'CloudGemPlayerAccount'

def add_transaction_handlers(handler_context, transaction_handlers):
    service_api_name = GEM_NAME + '.ServiceApi'
    base_url = handler_context.mappings.get(service_api_name, {}).get('PhysicalResourceId')
    if not base_url:
        raise RuntimeError('Missing PhysicalResourceId for ' + service_api_name)

    transaction_handlers.append(ServiceStatus(base_url))
    transaction_handlers.append(ChangePlayerName(base_url))
    transaction_handlers.append(GetAccount(base_url))

class ServiceStatus(ServiceApiCall):
    def __init__(self, base_url):
        ServiceApiCall.__init__(self, name=GEM_NAME + '.ServiceStatus', method='get', base_url=base_url, path='/service/status')

class GetAccount(ServiceApiCall):
    def __init__(self, base_url):
        ServiceApiCall.__init__(self, name=GEM_NAME + '.GetAccount', method='get', base_url=base_url, path='/account')

class ChangePlayerName(ServiceApiCall):
    def __init__(self, base_url):
        ServiceApiCall.__init__(self, name=GEM_NAME + '.ChangePlayerName', method='put', base_url=base_url, path='/account')

    def build_request(self):
        request = ServiceApiCall.build_request(self)
        request['body'] = {'PlayerName': 'Player' + str(randint(0,1000))}
        return request
