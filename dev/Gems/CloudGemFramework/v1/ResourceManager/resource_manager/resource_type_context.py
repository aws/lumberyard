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

import util

from resource_manager_common import resource_type_info
from resource_manager_common import stack_info

class ResourceTypeContext(object):
    def __init__(self, context):
        self.__context = context
        self.__resource_types_by_stack_id = {}

    def get_type_definitions_for_stack_id(self, stack_id, s3_client=None):
        result = self.__resource_types_by_stack_id.get(stack_id, None)

        if not result:
            # Load the type definitions for this stack and its ancestors from the configuration bucket
            session = self.__context.aws.session
            s3_client = self.__context.aws.client('s3') if s3_client is None else s3_client
            stack = self.__context.stack_info.manager.get_stack_info(stack_id, session)
            result = resource_type_info.load_resource_type_mapping(
                self.__context.config.configuration_bucket_name,
                stack,
                s3_client
            )

            # Cache the type definitions for this stack
            self.__resource_types_by_stack_id[stack_id] = result

        return result
