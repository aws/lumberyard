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
# $Revision: #7 $

import properties
import custom_resource_response
import discovery_utils

def handler(event, context):
    
    # This resource does nothing. It exists so that deployment stacks can be created
    # before any resource groups have been defined. In such cases the Resources list 
    # would be empty, which CloudFormation doesn't allow, so the lmbr_aws client inserts
    # this resource into the deployment template when no other resources are defined
    # by the template.

    props = properties.load(event, {})

    data = {}

    physical_resource_id = 'CloudCanvas:EmptyDeployment:{}'.format(discovery_utils.get_stack_name_from_stack_arn(event['StackId']))

    custom_resource_response.succeed(event, context, data, physical_resource_id)
