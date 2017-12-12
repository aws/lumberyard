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

import properties
import custom_resource_response
import discovery_utils

def handler(event, context):
    
    props = properties.load(event, {
        'ConfigurationBucket': properties.String(),
        'ConfigurationKey': properties.String(),
        'ResourceGroupName': properties.String()})

    data = {
        'ConfigurationBucket': props.ConfigurationBucket,
        'ConfigurationKey': '{}/resource-group/{}'.format(props.ConfigurationKey, props.ResourceGroupName),
        'TemplateURL': 'https://s3.amazonaws.com/{}/{}/resource-group/{}/resource-template.json'.format(props.ConfigurationBucket, props.ConfigurationKey, props.ResourceGroupName)
    }

    physical_resource_id = 'CloudCanvas:LambdaConfiguration:{stack_name}:{resource_group_name}'.format(
        stack_name=discovery_utils.get_stack_name_from_stack_arn(event['StackId']),
        resource_group_name=props.ResourceGroupName)

    custom_resource_response.succeed(event, context, data, physical_resource_id)
