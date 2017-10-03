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

import json
import httplib
from urlparse import urlparse

class HttpError(Exception):
    def __init__(self, response):
        self.message = 'Unexpected HTTP response status {} {}'.format(response.status, response.reason)


def succeed(event, context, data, physical_resource_id):
    print 'succeeded -- event: {} -- context: {} -- data: {} -- physical_resource_id: {}'.format(event, context, data, physical_resource_id)
    __send(event, context, 
        {
            'Status': 'SUCCESS',
            'PhysicalResourceId': physical_resource_id,
            'StackId': event['StackId'],
            'RequestId': event['RequestId'],
            'LogicalResourceId' : event['LogicalResourceId'],
            'Data': data
        }
    )


def fail(event, context, reason):
    print 'failed -- event: {} -- context: {} -- reason: {}'.format(event, context, reason)
    __send(event, context, 
        {
            'Status': 'FAILED',
            'StackId': event['StackId'],
            'RequestId': event['RequestId'],
            'LogicalResourceId' : event['LogicalResourceId'],
            'PhysicalResourceId' : event.get('PhysicalResourceId', event['RequestId']),
            'Reason': reason
        }
    )


def __send(event, context, body):

    bodyJSON = json.dumps(body)

    parsed_url = urlparse(event['ResponseURL'])

    path = parsed_url.path
    if parsed_url.query:
       path += '?' + parsed_url.query

    connection = httplib.HTTPSConnection(parsed_url.hostname)
    connection.connect()
    connection.request('PUT', path, bodyJSON)
    response = connection.getresponse()
    if response.status != httplib.OK:
        raise HttpError(response)


