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

import discovery_utils
import AccessResourceHandler

def handler(event, context):
    
    resource_properties = event['ResourceProperties']

    resource_properties['RoleLogicalId'] = 'Player'
    resource_properties['PhysicalResourceId'] = 'CloudCanvas:PlayerAccess:{stack_name}'.format(
        stack_name=discovery_utils.get_stack_name_from_stack_arn(event['StackId']))
    resource_properties['MetadataKey'] = 'PlayerAccess'
    resource_properties['UsePropagationDelay'] = 'false'
    resource_properties['RequireRoleExists'] = 'false'
        
    return AccessResourceHandler.handler(event, context)

