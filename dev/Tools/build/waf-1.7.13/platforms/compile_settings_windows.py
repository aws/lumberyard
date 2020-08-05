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
# Original file Copyright Crytek GMBH or its affiliates, used under license.
#

# System Imports
import os

# waflib imports
from waflib import Logs
from waflib.Configure import conf
from waflib.Errors import WafError


@conf
def load_windows_common_settings(conf):
    """
    Setup all compiler and linker settings shared over all windows configurations
    """
    v = conf.env
    
    # Configure manifest tool
    v.MSVC_MANIFEST = True  

    # Setup default libraries to always link
    v['LIB'] += [ 'User32',  'Advapi32', 'PsAPI' ]

    # Load Resource Compiler Tool
    conf.load_rc_tool()
    
    
@conf
def register_win_x64_external_ly_identity(self, compiler, configuration):

    # Do not register as an external library if the source exists
    if os.path.exists(self.Path('Code/Tools/LyIdentity/wscript')):
        return

    platform = 'windows'
    processor = 'intel64'

    if configuration not in ('Debug', 'Release'):
        raise WafError("Invalid configuration value {}", configuration)

    target_platform = 'win_x64'
    ly_identity_base_path = self.CreateRootRelativePath('Tools/InternalSDKs/LyIdentity')
    include_path = os.path.join(ly_identity_base_path, 'include')
    stlib_path = os.path.join(ly_identity_base_path, 'lib', platform, processor, configuration)
    shlib_path = os.path.join(ly_identity_base_path, 'bin', platform, processor, configuration)
    self.register_3rd_party_uselib('LyIdentity_shared',
                                   target_platform,
                                   includes=[include_path],
                                   defines=['LINK_LY_IDENTITY_DYNAMICALLY'],
                                   importlib=['LyIdentity_shared.lib'],
                                   sharedlibpath=[shlib_path],
                                   sharedlib=['LyIdentity_shared.dll'])

    self.register_3rd_party_uselib('LyIdentity_static',
                                   target_platform,
                                   includes=[include_path],
                                   libpath=[stlib_path],
                                   lib=['LyIdentity_static.lib'])


@conf
def register_win_x64_external_ly_metrics(self, compiler, configuration):

    # Do not register as an external library if the source exists
    if os.path.exists(self.Path('Code/Tools/LyMetrics/wscript')):
        return

    platform = 'windows'
    processor = 'intel64'

    if configuration not in ('Debug', 'Release'):
        raise WafError("Invalid configuration value {}", configuration)

    target_platform = 'win_x64'
    ly_identity_base_path = self.CreateRootRelativePath('Tools/InternalSDKs/LyMetrics')
    include_path = os.path.join(ly_identity_base_path, 'include')
    stlib_path = os.path.join(ly_identity_base_path, 'lib', platform, processor, configuration)
    shlib_path = os.path.join(ly_identity_base_path, 'bin', platform, processor, configuration)

    self.register_3rd_party_uselib('LyMetricsShared_shared',
                                   target_platform,
                                   includes=[include_path],
                                   defines=['LINK_LY_METRICS_DYNAMICALLY'],
                                   importlib=['LyMetricsShared_shared.lib'],
                                   sharedlibpath=[shlib_path],
                                   sharedlib=['LyMetricsShared_shared.dll'])

    self.register_3rd_party_uselib('LyMetricsShared_static',
                                   target_platform,
                                   includes=[include_path],
                                   libpath=[stlib_path],
                                   lib=['LyMetricsShared_static.lib'])

    self.register_3rd_party_uselib('LyMetricsProducer_shared',
                                   target_platform,
                                   includes=[include_path],
                                   defines=['LINK_LY_METRICS_PRODUCER_DYNAMICALLY'],
                                   importlib=['LyMetricsProducer_shared.lib'],
                                   sharedlibpath=[shlib_path],
                                   sharedlib=['LyMetricsProducer_shared.dll'])

    self.register_3rd_party_uselib('LyMetricsProducer_static',
                                   target_platform,
                                   includes=[include_path],
                                   libpath=[stlib_path],
                                   lib=['LyMetricsProducer_static.lib'])

@conf
def register_win_x64_external_optional_cuda(self, target_platform):

    # Obtain CUDA sdk path from system variable CUDA_PATH
    cuda_sdk_root = os.getenv('CUDA_PATH')
    if not cuda_sdk_root or not os.path.isdir(cuda_sdk_root) or not os.path.exists(cuda_sdk_root):
        return;

    Logs.info('[INFO] Detected NVIDIA CUDA SDK at: {}'.format(cuda_sdk_root))
    
    cuda_includes = os.path.join(cuda_sdk_root, 'include')
    cude_libpath = os.path.join(cuda_sdk_root, 'lib', 'x64')
    cuda_lib = 'cuda.lib'
    
    folders_to_validate = [cuda_includes, cude_libpath]
    for folder in folders_to_validate:
        if not os.path.isdir(folder) or not os.path.exists(folder):
            Logs.warn('[WARN] Missing NVIDIA CUDA SDK folder {}'.format(folder))
            Logs.warn('[WARN] NVIDIA CUDA SDK will not be used.')
            return;

    self.register_3rd_party_uselib('CUDA', target_platform,
                                   includes=[cuda_includes],
                                   defines=['CUDA_ENABLED'],
                                   libpath=[cude_libpath],
                                   lib=[cuda_lib],
                                   linkflags=['/DELAYLOAD:nvcuda.dll'])
