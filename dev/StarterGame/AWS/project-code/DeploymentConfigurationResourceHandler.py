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
        'DeploymentName': properties.String()})

    data = {
        'ConfigurationBucket': props.ConfigurationBucket,
        'ConfigurationKey': '{}/deployment/{}'.format(props.ConfigurationKey, props.DeploymentName),
        'DeploymentTemplateURL': 'https://s3.amazonaws.com/{}/{}/deployment/{}/deployment-template.json'.format(props.ConfigurationBucket, props.ConfigurationKey, props.DeploymentName),
        'AccessTemplateURL': 'https://s3.amazonaws.com/{}/{}/deployment-access-template-{}.json'.format(props.ConfigurationBucket, props.ConfigurationKey, props.DeploymentName),
        'DeploymentName': props.DeploymentName
    }

    physical_resource_id = 'CloudCanvas:DeploymentConfiguration:{}:{}'.format(discovery_utils.get_stack_name_from_stack_arn(event['StackId']), props.DeploymentName)

    custom_resource_response.succeed(event, context, data, physical_resource_id)
