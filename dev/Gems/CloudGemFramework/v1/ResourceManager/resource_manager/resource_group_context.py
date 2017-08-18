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

from errors import HandledError
from resource_group import ResourceGroup
import os
import constant

class ResourceGroupContext(object):

    def __init__(self, context):
        self.__context = context
        self.__resource_groups = {}

    def __iter__(self):
        return self.__resource_groups.iteritems()

    def __contains__(self, value):
        return value in self.__resource_groups

    def keys(self):
        return self.__resource_groups.keys()

    def values(self):
        return self.__resource_groups.values()

    def get(self, name, optional=False):
        result = self.__resource_groups.get(name, None)
        if result is None and not optional:
            raise HandledError('The resource group {} does not exist.'.format(name))
        return result

    def verify_exists(self, name):
        self.get(name, optional=False)

    def bootstrap(self, args):
        self.load_resource_groups()

    def initialize(self, args):
        self.bootstrap(args)

    def load_resource_groups(self):
        
        self.__resource_groups = {}

        for resource_group_name in self.__context.config.local_project_settings.get(constant.ENABLED_RESOURCE_GROUPS_KEY, []):
            self.__resource_groups[resource_group_name] = ResourceGroup(self.__context, resource_group_name = resource_group_name)

