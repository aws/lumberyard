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

from aws import AWSContext
from config import ConfigContext
from stack import StackContext
from view import ViewContext
from metrics import MetricsContext
from gem import GemContext
from resource_group_context import ResourceGroupContext
from resource_type_context import ResourceTypeContext
from stack_info_manager_context import StackInfoManagerContext
from hook import HookContext

class Context(object):

    '''Aggregates objects that provide a context for performing Lumberyard resource management operations.'''

    def __init__(self, metricsInterface, view_class=ViewContext):
        self.metrics = metricsInterface
        self.view = view_class(self)
        self.aws = AWSContext(self)
        self.stack = StackContext(self)
        self.config = ConfigContext(self)
        self.gem = GemContext(self)
        self.resource_groups = ResourceGroupContext(self)
        self.hooks = HookContext(self)
        self.resource_types = ResourceTypeContext(self)
        self.stack_info = StackInfoManagerContext(self)

    def bootstrap(self, args):
        self.view.bootstrap(args)
        self.config.bootstrap(args)
        self.gem.bootstrap(args)
        self.resource_groups.bootstrap(args)

    def initialize(self, args):
        self.view.initialize(args)
        self.aws.initialize(args)
        self.stack.initialize(args)
        self.config.initialize(args)
        self.resource_groups.initialize(args)

    def __str__(self):
        return '[ config: {config} ]'.format(config=self.config)
