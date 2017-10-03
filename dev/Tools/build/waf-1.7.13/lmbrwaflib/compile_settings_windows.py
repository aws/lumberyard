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
from waflib.Configure import conf

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
def load_debug_windows_settings(conf):
    """
    Setup all compiler and linker settings shared over all windows configurations for
    the 'debug' configuration
    """
    conf.load_windows_common_settings()
    
@conf
def load_profile_windows_settings(conf):
    """
    Setup all compiler and linker settings shared over all windows configurations for
    the 'debug' configuration
    """
    conf.load_windows_common_settings()
    
@conf
def load_performance_windows_settings(conf):
    """
    Setup all compiler and linker settings shared over all windows configurations for
    the 'debug' configuration
    """
    conf.load_windows_common_settings()
    
@conf
def load_release_windows_settings(conf):
    """
    Setup all compiler and linker settings shared over all windows configurations for
    the 'debug' configuration
    """
    conf.load_windows_common_settings()
