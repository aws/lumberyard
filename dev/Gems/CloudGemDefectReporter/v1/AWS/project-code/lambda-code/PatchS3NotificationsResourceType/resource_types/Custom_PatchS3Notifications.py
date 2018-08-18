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

import os
import boto3
from cgf_utils import custom_resource_response
import defect_reporter_s3 as s3
import defect_reporter_lambda as lambda_



def handler(event, context):
    ''' Invoked when AWS Lambda service executes. '''

    if not __is_valid_event(event):
        return custom_resource_response.failure_response('Malformed event recieved.')

    request_type = event['RequestType']

    if request_type != 'Create':
        print( 'Saw RequestType: \"{}\". No action needed (Only \"Create\" supported)'.format(request_type))
        return custom_resource_response.success_response({}, '*')

    s3_client = s3.get_client()
    lambda_client = lambda_.get_client()

    bucket_name, lambda_arn = __get_resources(event)

    has_permission =  __add_permission_to_trigger(lambda_client, lambda_arn, bucket_name)

    if not has_permission:
        return custom_resource_response.failure_response('Could not add permissions to Lambda')
    
    is_configured = __add_notification_configuration(bucket_name, lambda_arn, s3_client)

    if is_configured:
        return custom_resource_response.success_response({}, '*')
    else:
        return custom_resource_response.failure_response('Could not succesfully configure AttachmentBucket')


def __is_valid_event(event):
    ''' Validation on incoming event. ''' 

    if 'RequestType' not in event:
        return False

    if 'ResourceProperties' not in event:
        return False

    return True


def __add_permission_to_trigger(lambda_client, lambda_name, bucket_name):
    '''' Adds necessary permissions to trigger lambda to allow it to intract with S3 client. '''
    
    try:
        lambda_client.add_permission(
            FunctionName=lambda_name,
            StatementId='s3invoke',
            Action='lambda:InvokeFunction',
            Principal='s3.amazonaws.com',
            SourceArn="arn:aws:s3:::"+bucket_name
        )
    except Exception as e:
        print("[ERROR] {}".format(e))
        return False
    else:
        return True
    


def __add_notification_configuration(bucket_name, lambda_arn, s3_client):
    ''' Put notification confugurations on bucket to enable object creation triggers. '''

    try:
        s3_client.put_bucket_notification_configuration(
            Bucket=bucket_name, 
            NotificationConfiguration={'LambdaFunctionConfigurations':[{'LambdaFunctionArn': lambda_arn, 'Events':['s3:ObjectCreated:*']}]})
    except RuntimeError:
        return False
    else:
        return True


def __get_resources(event):
    ''' Retrieve bucket name and lambda arn from event. '''

    resource_properties = event['ResourceProperties']

    if 'AttachmentBucket' in resource_properties:
        bucket_name = resource_properties['AttachmentBucket']
    else:
        raise RuntimeError('Malformed resource properties (Bucket ARN).')

    if 'SanitizationLambdaArn' in resource_properties:
        lambda_arn = resource_properties['SanitizationLambdaArn']
    else:
        raise RuntimeError('Malformed resource properties (Lambda ARN).')

    return bucket_name, lambda_arn
