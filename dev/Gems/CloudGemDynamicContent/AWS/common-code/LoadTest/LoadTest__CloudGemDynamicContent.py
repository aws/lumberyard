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
from random import choice
from requests_aws4auth import AWS4Auth

import requests
import dynamic_content_constant as c


#
# Load Test Transaction Handler registration
#
def add_transaction_handlers(handler_context, transaction_handlers):
    service_api_name = c.GEM_NAME + '.ServiceApi'
    base_url = handler_context.mappings.get(service_api_name, {}).get('PhysicalResourceId')
    if not base_url:
        raise RuntimeError('Missing PhysicalResourceId for ' + service_api_name)

    transaction_handlers.append(ServiceStatus(base_url))
    transaction_handlers.append(GetSingleFile(base_url))
    transaction_handlers.append(GetThreeFiles(base_url))
    transaction_handlers.append(DownloadSinglePak(base_url))


#
# Check for the service status of Cloud Gem Under Test
#
class ServiceStatus(ServiceApiCall):
    def __init__(self, base_url):
        ServiceApiCall.__init__(self, name=c.GEM_NAME + '.ServiceStatus', method='get', base_url=base_url, path='/service/status')


#
# Get single file from user API
#
class GetSingleFile(ServiceApiCall):
    def __init__(self, base_url):
        # This file selection is only done once.
        body = {'FileList': [choice(c.TEST_FILENAMES)]}
        ServiceApiCall.__init__(self, name=c.GEM_NAME + '.GetSingleFile', method='post', base_url=base_url, path='/client/content', body=body)


#
# Get Three Files from user API
#
class GetThreeFiles(ServiceApiCall):
    def __init__(self, base_url):
        body = {'FileList': c.TEST_FILENAMES}
        ServiceApiCall.__init__(self, name=c.GEM_NAME + '.GetThreeFiles', method='post', base_url=base_url, path='/client/content', body=body)


#
# Download a single Dynamic Content Pak file from user API
#
class DownloadSinglePak(ServiceApiCall):
    def __init__(self, base_url):
        ServiceApiCall.__init__(self, name=c.GEM_NAME + '.DownloadSinglePak', base_url=base_url)

    def initialize(self, initialize_context):
        super(DownloadSinglePak, self).initialize(initialize_context)

        creds = initialize_context.player_credentials_provider.get()
        auth = AWS4Auth(creds['AccessKeyId'], creds['SecretKey'], creds['region'], 'execute-api', session_token=creds['SessionToken'])
        response = requests.request('post', self.base_url + '/client/content', auth=auth, json={'FileList': c.TEST_FILENAMES})
        self.__download_urls = [file['PresignedURL'] for file in response.json()['result']['ResultList']]

    def build_request(self):
        return {
            'url': self.__download_urls[0],
            'method': 'get'
        }
