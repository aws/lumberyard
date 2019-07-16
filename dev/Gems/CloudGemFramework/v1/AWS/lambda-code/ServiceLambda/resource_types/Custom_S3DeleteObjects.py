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
import boto3

from cgf_utils import aws_utils
from cgf_utils import custom_resource_response


from botocore.exceptions import ClientError
from botocore.client import Config

def handler(event, context):
    properties = event["ResourceProperties"]
    stack_arn = event['StackId']
    physical_resource_id = aws_utils.get_stack_name_from_stack_arn(
            stack_arn) + '-' + event['LogicalResourceId']

    buckets = properties.get("Buckets", [])
    if not buckets:
        print "There were no buckets in the resource properties, returning early"
        return custom_resource_response.success_response({}, physical_resource_id)

    if event["RequestType"] != "Delete":
        return custom_resource_response.success_response({}, physical_resource_id)

    for bucket_name in buckets:
        clear_bucket(bucket_name)
    return custom_resource_response.success_response({}, physical_resource_id)


def clear_bucket(bucket_name):
    try:
        print "clearing bucket {}".format(bucket_name)
        bucket = boto3.resource('s3', config=Config(signature_version='s3v4')).Bucket(bucket_name)
        bucket.object_versions.delete()
    except ClientError as err:
        if err.response['Error']['Code'] in ['NoSuchBucket']:
            print "Bucket does not exist"
        else:
            raise err
