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

from lmbr_install_context import LmbrInstallContext
from build_configurations import PLATFORM_MAP


class RunUnitTestContext(LmbrInstallContext):
    fun = 'run_unit_test'
    group_name = 'run_test'

    def run_unit_test(self, **kw):
        """
        Creates a run test task
        """
        self.process_restricted_settings(kw)
        if self.is_platform_and_config_valid(**kw):
            self(features='unittest_{}'.format(self.platform), group=self.group_name)


for platform_name, platform in PLATFORM_MAP.items():
    for configuration in platform.get_configuration_names():
        
        configuration_details = platform.get_configuration(configuration)
        if not configuration_details.is_test:
            # Skip any non-test configurations
            continue
       
        platform_config_key = '{}_{}'.format(platform_name, configuration)

        # Create new class to execute run_test command with variant
        class_attributes = {
            'cmd'           : 'run_{}'.format(platform_config_key),
            'variant'       : platform_config_key,
            'platform'      : platform_name,
            'config'        : configuration,
        }

        subclass = type('{}{}RunUnitTestContext'.format(platform_name.title(), configuration.title()), (RunUnitTestContext,), class_attributes)
