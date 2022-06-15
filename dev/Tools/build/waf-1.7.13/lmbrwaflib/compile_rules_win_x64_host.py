﻿#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

# System Imports
import os

# waflib imports
from waflib.Configure import conf, Logs


# being a _host file, this means that these settings apply to any build at all that is
# being done from this kind of host

AZCG_VALIDATED_PATH = None
AZ_CODE_GEN_EXECUTABLE = 'AzCodeGenerator.exe'


###############################################################################
# DX12 detection and configuration
DX12_INCLUDE_DIR = {}
DX12_LIB_DIR = {}

@conf
def find_dx12(conf, windows_kit):
    global DX12_INCLUDE_DIR
    global DX12_LIB_DIR
    if not windows_kit in DX12_INCLUDE_DIR:
        DX12_INCLUDE_DIR[windows_kit] = None
    if not windows_kit in DX12_LIB_DIR:
        DX12_LIB_DIR[windows_kit] = None
    if (DX12_INCLUDE_DIR[windows_kit] is None) or (DX12_LIB_DIR[windows_kit] is None):
        DX12_INCLUDE_DIR[windows_kit] = []   # set to empty, will prevent this part of the function from running more than once even if nothing is found
        DX12_LIB_DIR[windows_kit] = []
        v = conf.env
        includes = v['INCLUDES']
        libs = v['LIBPATH']

        for path in includes:
            if windows_kit in path:
                for root, dirs, files in os.walk(path):
                    if 'd3d12.h' in files:
                        DX12_INCLUDE_DIR[windows_kit] = root
                        break
                if DX12_INCLUDE_DIR[windows_kit]:
                    break

        # this is expensive, so only do it if the include was also found
        if DX12_INCLUDE_DIR[windows_kit]:
            for path in libs:
                if windows_kit in path:
                    for root, dirs, files in os.walk(path):
                        if 'd3d12.lib' in files:
                            Logs.debug('[DX12] libraries found, can compile for DX12')
                            DX12_LIB_DIR[windows_kit] = root
                            break
                    if DX12_LIB_DIR[windows_kit]:
                        break

    conf.env['DX12_INCLUDES'] = DX12_INCLUDE_DIR[windows_kit]
    conf.env['DX12_LIBPATH'] = DX12_LIB_DIR[windows_kit]


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
        az_code_gen_subfolders = ['bin/windows']
        validated_azcg_dir = None
        for az_code_gen_subfolder in az_code_gen_subfolders:
            azcg_dir = conf.Path('Tools/AzCodeGenerator/{}'.format(az_code_gen_subfolder))
            azcg_exe = os.path.join(azcg_dir, AZ_CODE_GEN_EXECUTABLE)
            if os.path.exists(azcg_exe):
                Logs.debug('lumberyard: Found AzCodeGenerator at {}'.format(azcg_dir))
                validated_azcg_dir = azcg_dir
                break
        AZCG_VALIDATED_PATH = validated_azcg_dir
        if validated_azcg_dir is None:
            conf.fatal('Unable to locate the AzCodeGenerator subfolder.  Make sure that the Windows binaries are available')

    v['CODE_GENERATOR_EXECUTABLE'] = AZ_CODE_GEN_EXECUTABLE
    v['CODE_GENERATOR_PATH'] = [AZCG_VALIDATED_PATH]
    v['CODE_GENERATOR_PYTHON_PATHS'] = [conf.Path('Tools/Python/3.7.10/windows/Lib'),
                                        conf.Path('Tools/Python/3.7.10/windows/libs'),
                                        conf.Path('Tools/Python/3.7.10/windows/DLLs'),
                                        conf.ThirdPartyPath('markupsafe', 'x64'),
                                        conf.ThirdPartyPath('jinja2', 'x64')]
    v['CODE_GENERATOR_PYTHON_DEBUG_PATHS'] = [conf.Path('Tools/Python/3.7.10/windows/Lib'),
                                              conf.Path('Tools/Python/3.7.10/windows/libs'),
                                              conf.Path('Tools/Python/3.7.10/windows/DLLs'),
                                              conf.ThirdPartyPath('markupsafe', 'x64'),
                                              conf.ThirdPartyPath('jinja2', 'x64')]
    v['EMBEDDED_PYTHON_HOME_RELATIVE_PATH'] = 'Tools/Python/3.7.10/windows'
    v['CODE_GENERATOR_PYTHON_HOME'] = conf.Path(v['EMBEDDED_PYTHON_HOME_RELATIVE_PATH'])
    v['CODE_GENERATOR_PYTHON_HOME_DEBUG'] = conf.Path('Tools/Python/3.7.10/windows')
    v['CODE_GENERATOR_INCLUDE_PATHS'] = []
    
    v['EMBEDDED_PYTHON_HOME'] = v['CODE_GENERATOR_PYTHON_HOME']
    v['EMBEDDED_PYTHON_INCLUDE_PATH'] = os.path.join(v['EMBEDDED_PYTHON_HOME'],'include')
    v['EMBEDDED_PYTHON_LIBPATH'] = os.path.join(v['EMBEDDED_PYTHON_HOME'], 'libs')
    v['EMBEDDED_PYTHON_SHARED_OBJECT'] = os.path.join(v['EMBEDDED_PYTHON_HOME'], 'python37.dll')


    crcfix_dir = conf.Path('Tools/crcfix/bin/windows')
    if not os.path.exists(crcfix_dir):
        Logs.warn('Unable to locate the crcfix subfolder.  Make sure that the Windows crcfix binaries are available')
    v['CRCFIX_PATH'] = [crcfix_dir]
    v['CRCFIX_EXECUTABLE'] = 'crcfix.exe'

