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
from __future__ import absolute_import
from __future__ import print_function

import json
from cgf_utils import custom_resource_response
import defect_reporter_s3 as s3
import defect_reporter_lambda as lambda_


def empty_handler(event, context):
    return custom_resource_response.success_response({}, '*')


def handler(event, context):
    """ Invoked for PatchS3NotificationsResourceType custom cloudformation resource types """

    if not __is_valid_event(event):
        return custom_resource_response.failure_response('Malformed event received.')

    request_type = event['RequestType']

    if request_type != 'Create':
        print('Saw RequestType: \"{}\". No action needed (Only \"Create\" supported)'.format(request_type))
        return custom_resource_response.success_response({}, '*')

    s3_client = s3.get_client()
    lambda_client = lambda_.get_client()

    bucket_name, lambda_arn = __get_resources(event)

    has_permission = __add_permission_to_trigger(lambda_client, lambda_arn, bucket_name)

    if not has_permission:
        return custom_resource_response.failure_response('Could not add permissions to Lambda')

    is_configured = __add_notification_configuration(bucket_name, lambda_arn, s3_client)

    if is_configured:
        return custom_resource_response.success_response({}, '*')
    else:
        return custom_resource_response.failure_response('Could not successfully configure AttachmentBucket')

def __is_valid_event(event):
    """ Validation on incoming event. """

    if 'RequestType' not in event:
        return False

    if 'ResourceProperties' not in event:
        return False

    return True


def __compare_lambda_policy_statement(statement, action, principal, resource_arn):
    # Check principal matches
    if 'Principal' in statement and statement['Principal']['Service'] == principal:
        # Check action is correct
        if 'Action' in statement and statement['Action'] == action:
            # Check condition uses the same source arn
            if 'Condition' in statement and 'ArnLike' in statement['Condition']:
                s3_policy_arn = statement['Condition']['ArnLike']['AWS:SourceArn']
                if s3_policy_arn == resource_arn:
                    # S3 trigger policy matches
                    return True
    return False


def __find_existing_s3_trigger_statement(lambda_client, lambda_name, s3_arn):
    try:
        response = lambda_client.get_policy(FunctionName=lambda_name)
        print("response: {}".format(response))
        policy = json.loads(response['Policy'])
        print("policy: {}".format(policy))
        for statement in policy['Statement']:
            if __compare_lambda_policy_statement(statement=statement, action='lambda:InvokeFunction',
                                                 principal='s3.amazonaws.com', resource_arn=s3_arn):
                return statement

    except Exception as e:
        print("[ERROR] {}".format(e))
    return None


def __add_permission_to_trigger(lambda_client, lambda_name, bucket_name):
    """
    Adds necessary permissions to trigger lambda to allow it to interact with S3 client, if permissions are missing

    Note: Restricts trigger to an S3 bucket in the same account.
    """
    bucket_arn = "arn:aws:s3:::" + bucket_name
    try:
        # Check to see if have existing statement in place for s3 trigger
        statement = __find_existing_s3_trigger_statement(
            lambda_client=lambda_client, lambda_name=lambda_name, s3_arn=bucket_arn)

        # If no existing statement exists - add it in
        if statement is None:
            lambda_client.add_permission(
                FunctionName=lambda_name,
                StatementId='s3invoke_patch_s3_notifications',
                Action='lambda:InvokeFunction',
                Principal='s3.amazonaws.com',
                SourceArn=bucket_arn,
                SourceAccount=__get_owning_account_from_arn(lambda_name)
            )
        else:
            print("Lambda policy found that already granted S3 permissions {}. No update applied".format(statement))

    except Exception as e:
        print("[ERROR] {}".format(e))
        return False
    else:
        return True


def __add_notification_configuration(bucket_name, lambda_arn, s3_client):
    """ Put notification configurations on bucket to enable object creation triggers. """

    try:
        s3_client.put_bucket_notification_configuration(
            Bucket=bucket_name,
            NotificationConfiguration={'LambdaFunctionConfigurations': [{'LambdaFunctionArn': lambda_arn, 'Events': ['s3:ObjectCreated:*']}]})
    except RuntimeError:
        return False
    else:
        return True


def __get_resources(event):
    """ Retrieve bucket name and lambda arn from event. """

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


def __get_owning_account_from_arn(arn):
    """
    Get the owning aws account id part of the arn
    """
    if arn is not None:
        return arn.split(':')[4]
    return None
