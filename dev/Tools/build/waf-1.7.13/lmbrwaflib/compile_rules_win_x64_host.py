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
from waflib.Configure import conf, Logs
import cry_utils
import os


# being a _host file, this means that these settings apply to any build at all that is
# being done from this kind of host

AZCG_VALIDATED_PATH = None
AZ_CODE_GEN_EXECUTABLE = 'AzCodeGenerator.exe'


@conf
def load_win_x64_host_settings(conf):
    """
    Setup any environment settings you want to apply globally any time the host doing the building is win x64
    """
    v = conf.env

    # Setup the environment for the AZ Code Generator

    # Look for the most recent version of the code generator subfolder.  This should be either installed or built by the bootstrap process at this point
    global AZCG_VALIDATED_PATH
    if AZCG_VALIDATED_PATH is None:
        az_code_gen_subfolders = ['bin/vc140', 'bin/vc120']
        validated_azcg_dir = None
        for az_code_gen_subfolder in az_code_gen_subfolders:
            azcg_dir = conf.srcnode.make_node('Tools/AzCodeGenerator/{}'.format(az_code_gen_subfolder)).abspath()
            azcg_exe = os.path.join(azcg_dir, AZ_CODE_GEN_EXECUTABLE)
            if os.path.exists(azcg_exe):
                Logs.debug('lumberyard: Found AzCodeGenerator at {}'.format(azcg_dir))
                validated_azcg_dir = azcg_dir
                break
        AZCG_VALIDATED_PATH = validated_azcg_dir
        if validated_azcg_dir is None:
            conf.fatal('Unable to locate the AzCodeGenerator subfolder.  Make sure that you have either the VS2013 or VS2015 binaries available')

    v['CODE_GENERATOR_EXECUTABLE'] = AZ_CODE_GEN_EXECUTABLE
    v['CODE_GENERATOR_PATH'] = [AZCG_VALIDATED_PATH]
    v['CODE_GENERATOR_PYTHON_PATHS'] = ['Tools/Python/2.7.12/windows/Lib', 'Tools/Python/2.7.12/windows/libs', 'Tools/Python/2.7.12/windows/DLLs', 'Code/SDKs/markupsafe/x64', 'Code/SDKs/jinja2/x64']
    v['CODE_GENERATOR_PYTHON_DEBUG_PATHS'] = ['Tools/Python/2.7.12/windows/Lib', 'Tools/Python/2.7.12/windows/libs', 'Tools/Python/2.7.12/windows/DLLs', 'Code/SDKs/markupsafe/x64', 'Code/SDKs/jinja2/x64']
    v['CODE_GENERATOR_PYTHON_HOME'] = 'Tools/Python/2.7.12/windows'
    v['CODE_GENERATOR_PYTHON_HOME_DEBUG'] = 'Tools/Python/2.7.12/windows'

    v['CRCFIX_EXECUTABLE'] = 'crcfix.exe'

