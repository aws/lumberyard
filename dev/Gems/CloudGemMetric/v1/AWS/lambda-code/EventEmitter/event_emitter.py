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
import cgf_lambda_settings
import cgf_service_client
import boto3
import metric_constant as c
import json

# This Lambda function sends corresponding metrics to all the other gems which have interfaces with a specific name started with "MetricsListener"
def main(message, context):
    interface_urls = cgf_lambda_settings.list_service_urls()
    event = message['emitted']
    
    for interface_url in interface_urls:
        if c.INT_METRICS_LISTENER in interface_url.keys()[0]:
            service_client = cgf_service_client.for_url(interface_url.values()[0], verbose=True, session=boto3._get_default_session())
            source = service_client.navigate('source').GET()

            if event.get('source', None) == source.name:
                print "#######Emitting"
                service_client.navigate('emit').POST(message)