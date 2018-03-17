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

from cgf_utils import aws_utils

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


sqs_client = aws_utils.ClientWrapper(boto3.client('sqs'))


def arn_handler(event, context):
    resource_name = event['ResourceName']
    result = sqs_client.get_queue_attributes(QueueUrl=resource_name, AttributeNames=["QueueArn"])
    queue_arn = result["Attributes"].get("QueueArn", None)
    if queue_arn is None:
        raise RuntimeError('Could not find QueueArn in {} for {}'.format(result, resource_name))
    result = {
        'Arn': queue_arn
    }

    return result
