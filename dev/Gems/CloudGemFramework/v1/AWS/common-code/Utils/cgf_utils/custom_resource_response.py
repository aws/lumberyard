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
from urlparse import urlparse
import random
import time
import json
from cgf_utils import json_utils


def success_response(data, physical_resource_id):
    return {
        'Success': True,
        'Data': data,
        'PhysicalResourceId': physical_resource_id
    }


def failure_response(reason):
    return {
        'Success': False,
        'Reason': reason
    }


class HttpError(Exception):
    def __init__(self, response):
        self.message = 'Unexpected HTTP response status {} {}'.format(response.status, response.reason)


def succeed(event, context, data, physical_resource_id):
    print 'succeeded -- event: {} -- context: {} -- data: {} -- physical_resource_id: {}'.format(json.dumps(event, cls=json_utils.SafeEncoder), context, data, physical_resource_id)
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
    print 'failed -- event: {} -- context: {} -- reason: {}'.format(json.dumps(event, cls=json_utils.SafeEncoder), context, reason)
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

BACKOFF_BASE_SECONDS = 0.25
BACKOFF_MAX_SECONDS = 60.0
BACKOFF_MAX_TRYS = 15

def __send(event, context, body):

    bodyJSON = json.dumps(body)

    parsed_url = urlparse(event['ResponseURL'])

    path = parsed_url.path
    if parsed_url.query:
       path += '?' + parsed_url.query

    # http://www.awsarchitectureblog.com/2015/03/backoff.html
    sleep_seconds = BACKOFF_BASE_SECONDS
    count = 1
    while True:

        try:

            connection = httplib.HTTPSConnection(parsed_url.hostname)
            connection.connect()
            connection.request('PUT', path, bodyJSON)
            response = connection.getresponse()
            if response.status != httplib.OK:
                raise HttpError(response)

            return

        except Exception as e:

            print 'Attempt to send custom resource response failed: {}'.format(e.message)

            if count == BACKOFF_MAX_TRYS:
                raise e

            temp = min(BACKOFF_MAX_SECONDS, BACKOFF_BASE_SECONDS * 2 ** count)
            sleep_seconds = temp / 2 + random.uniform(0, temp / 2)

            print 'Retry attempt {}. Sleeping {} seconds'.format(count, sleep_seconds)

            time.sleep(sleep_seconds)

            count += 1




