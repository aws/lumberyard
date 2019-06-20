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
import os
import plistlib
import shutil
import subprocess

from lmbr_install_context import LmbrInstallContext
from build_configurations import PLATFORM_MAP


class DeployContext(LmbrInstallContext):
    fun = 'deploy'
    group_name = 'deploy'

    def deploy_game_legacy(self, **kw):
        """
        Creates a deploy task using the old feature method
        """
        if self.is_platform_and_config_valid(**kw):
            self(features='deploy_{}'.format(self.platform), group=self.group_name)


for platform_name, platform in PLATFORM_MAP.items():
    for configuration in platform.get_configuration_names():
        platform_config_key = '{}_{}'.format(platform_name, configuration)

        # Create new class to execute deploy command with variant
        class_attributes = {
            'cmd'           : 'deploy_{}'.format(platform_config_key),
            'variant'       : platform_config_key,
            'platform'      : platform_name,
            'config'        : configuration,
        }

        subclass = type('{}{}DeployContext'.format(platform_name.title(), configuration.title()), (DeployContext,), class_attributes)
