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

import random
import time
import json
import datetime
import os
import boto3

import json_utils

from botocore.exceptions import ClientError
from botocore.exceptions import EndpointConnectionError
from botocore.exceptions import IncompleteReadError
from botocore.exceptions import ConnectionError
from botocore.exceptions import BotoCoreError
from botocore.exceptions import UnknownEndpointError

current_region = os.environ.get('AWS_REGION')

class ClientWrapper(object):

    BACKOFF_BASE_SECONDS = 0.25
    BACKOFF_MAX_SECONDS = 60.0
    BACKOFF_MAX_TRYS = 15

    def __init__(self, wrapped_client):
        self.__wrapped_client = wrapped_client
        self.__client_type = type(wrapped_client).__name__

    @property
    def client_type(self):
        return self.__client_type

    def __getattr__(self, attr):
        orig_attr = self.__wrapped_client.__getattribute__(attr)
        if callable(orig_attr):

            def client_wrapper(*args, **kwargs):
                # http://www.awsarchitectureblog.com/2015/03/backoff.html
                sleep_seconds = self.BACKOFF_BASE_SECONDS
                count = 1
                while True:
                    self.__log_attempt(attr, args, kwargs)
                    try:
                        result = orig_attr(*args, **kwargs)
                        self.__log_success(attr, result)
                        return result
                    except (ClientError,EndpointConnectionError,IncompleteReadError,ConnectionError,BotoCoreError,UnknownEndpointError) as e:

                        self.__log_failure(attr, e)

                        if count == self.BACKOFF_MAX_TRYS or (isinstance(e, ClientError) and e.response['Error']['Code'] not in ['Throttling', 'TooManyRequestsException']):
                            raise e

                        temp = min(self.BACKOFF_MAX_SECONDS, self.BACKOFF_BASE_SECONDS * 2 ** count)
                        sleep_seconds = temp / 2 + random.uniform(0, temp / 2)

                        self.__log(attr, 'Retry attempt {}. Sleeping {} seconds'.format(count, sleep_seconds))

                        time.sleep(sleep_seconds)

                        count += 1

                    except Exception as e:
                        self.__log_failure(attr, e)
                        raise e

            return client_wrapper

        else:
            return orig_attr

    def __log(self, method_name, log_msg):

        msg = 'AWS '
        msg += self.__client_type
        msg += '.'
        msg += method_name
        msg += ' '
        msg += log_msg

        print msg


    def __log_attempt(self, method_name, args, kwargs):

        msg = 'attempt: '

        comma_needed = False
        for arg in args:
            if comma_needed: msg += ', '
            msg += arg
            msg += type(arg).__name__
            comma_needed = True

        for key,value in kwargs.iteritems():
            if comma_needed: msg += ', '
            msg += key
            msg += type(value).__name__
            msg += '='
            if isinstance(value, basestring):
                msg += '"'
                msg += value
                msg += '"'
            elif isinstance(value, dict):
                msg += json.dumps(value, cls=json_utils.JSONCustomEncoder)
            else:
                msg += str(value)
            comma_needed = True

        self.__log(method_name, msg)


    def __log_success(self, method_name, result):

        msg = 'success: '
        msg += type(result).__name__
        if isinstance(result, dict):
            msg += json.dumps(result, cls=json_utils.JSONCustomEncoder)
        else:
            msg += str(result)

        self.__log(method_name, msg)


    def __log_failure(self, method_name, e):

        msg = ' failure '
        msg += type(e).__name__
        msg += ': '
        msg += str(getattr(e, 'response', e.message))

        self.__log(method_name, msg)

ID_DATA_MARKER = '::'

def get_data_from_custom_physical_resource_id(physical_resource_id):
    '''Returns data extracted from a physical resource id with embedded JSON data.'''
    if physical_resource_id:
        i_data_marker = physical_resource_id.find(ID_DATA_MARKER)
        if i_data_marker == -1:
            id_data = {}
        else:
            try:
                id_data = json.loads(physical_resource_id[i_data_marker+len(ID_DATA_MARKER):])
            except Exception as e:
                print 'Could not parse JSON data from physical resource id {}. {}'.format(physical_resource_id, e.message)
                id_data = {}
    else:
        id_data = {}
    return id_data

       
def construct_custom_physical_resource_id_with_data(stack_arn, logical_resource_id, id_data):
    '''Creates a physical resource id with embedded JSON data.'''
    physical_resource_name = get_stack_name_from_stack_arn(stack_arn) + '-' + logical_resource_id
    id_data_string = json.dumps(id_data, sort_keys=True)
    return physical_resource_name + ID_DATA_MARKER + id_data_string
    
# Stack ARN format: arn:aws:cloudformation:{region}:{account}:stack/{name}/{uuid}

def get_stack_name_from_stack_arn(arn):
    return arn.split('/')[1]


def get_region_from_stack_arn(arn):
    return arn.split(':')[3]


def get_account_id_from_stack_arn(arn):
    return arn.split(':')[4]

def get_cloud_canvas_metadata(resource, metadata_name):

    metadata_string = resource.get('Metadata', None)
    if metadata_string is None: return

    try:
        metadata = json.loads(metadata_string)
    except ValueError as e:
        raise RuntimeError('Could not parse CloudCanvas {} Metadata: {}. {}'.format(metadata_name, metadata_string, e))

    cloud_canvas_metadata = metadata.get('CloudCanvas', None)
    if cloud_canvas_metadata is None: return

    return cloud_canvas_metadata.get(metadata_name, None)


RESOURCE_ARN_PATTERNS = {
    'AWS::DynamoDB::Table': 'arn:aws:dynamodb:{region}:{account_id}:table/{resource_name}',
    'AWS::Lambda::Function': 'arn:aws:lambda:{region}:{account_id}:function:{resource_name}',
    'AWS::SQS::Queue': '{resource_name}',
    'AWS::SNS::Topic': 'arn:aws:sns:{region}:{account_id}:{resource_name}',
    'AWS::S3::Bucket': 'arn:aws:s3:::{resource_name}',
    'Custom::CognitoUserPool': 'arn:aws:cognito-idp:{region}:{account_id}:userpool/{resource_name}',
    'Custom::ServiceApi': 'arn:aws:execute-api:{region}:{account_id}:{resource_name}',
    'Custom::Polly': "*",
    'Custom::Lex': '*'
}


def get_resource_arn(stack_arn, resource_type, resource_name):

    # TODO: need a way to "plug in" resource types, so hacks like this aren't necessary
    if resource_type == 'Custom::ServiceApi':
        id_data = get_data_from_custom_physical_resource_id(resource_name)
        rest_api_id = id_data.get('RestApiId', '')
        resource_name = rest_api_id
    elif resource_type == 'AWS::SQS::Queue':
        client = boto3.client('sqs')
        result = client.get_queue_attributes(QueueUrl=resource_name, AttributeNames=["QueueArn"])
        queue_arn = result["Attributes"].get("QueueArn", None)
        if queue_arn is None:
            raise RuntimeError('Could not find QueueArn in {} for {}'.format(result, resource_name))
        resource_name = queue_arn

    pattern = RESOURCE_ARN_PATTERNS.get(resource_type, None)
    if pattern is None:
        raise RuntimeError('Unsupported ARN mapping for resource type {} on resource {}. To add support for additional resource types, add an entry to RESOURCE_ARN_PATTERNS in {}.'.format(resource_type, resource_name, __file__))

    return pattern.format(
        region=get_region_from_stack_arn(stack_arn),
        account_id=get_account_id_from_stack_arn(stack_arn),
        resource_name=resource_name)
