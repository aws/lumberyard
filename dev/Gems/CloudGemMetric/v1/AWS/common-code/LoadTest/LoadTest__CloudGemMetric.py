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

from __future__ import print_function
from cloud_gem_load_test.service_api_call import ServiceApiCall
from data_generator import DataGenerator
import metric_constant as c

#
# Load Test Transaction Handler registration
#
def add_transaction_handlers(handler_context, transaction_handlers):
    service_api_name = c.RES_GEM_NAME + '.ServiceApi'
    base_url = handler_context.mappings.get(service_api_name, {}).get('PhysicalResourceId')
    if not base_url:
        raise RuntimeError('Missing PhysicalResourceId for ' + service_api_name)

    transaction_handlers.append(ServiceStatus(base_url))
    transaction_handlers.append(ProduceMessage(base_url))


#
# Check for the service status of Cloud Gem Under Test
#
class ServiceStatus(ServiceApiCall):
    def __init__(self, base_url):
        ServiceApiCall.__init__(self, name=c.RES_GEM_NAME + '.ServiceStatus', method='get', base_url=base_url,
                                path='/service/status')


#
# Produce Metric Messages
#
class ProduceMessage(ServiceApiCall):
    def __init__(self, base_url):
        ServiceApiCall.__init__(self, name=c.RES_GEM_NAME + '.ProduceMessage', method='post', base_url=base_url,
                                path='/producer/produce/message?compression_mode=NoCompression&sensitivity_type=Insensitive&payload_type=JSON')

    def build_request(self):
        request = ServiceApiCall.build_request(self)

        request['body'] = {
            'data': build_metric_data()
        }
        return request


#
# Build the metric data object needed for the metric producer request body
#
def build_metric_data():
    print('Building metric event data')    
    data_generator = DataGenerator()
    return data_generator.json(1)
