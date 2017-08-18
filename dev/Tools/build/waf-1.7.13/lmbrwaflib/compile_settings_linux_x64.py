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

def load_linux_x64_common_settings(v):
    """
    Setup all compiler and linker settings shared over all linux_x64 configurations
    """
    
    # Add common linux x64 defines
    v['DEFINES'] += [ 'LINUX64' ]   
    
@conf
def load_debug_linux_x64_settings(conf):
    """
    Setup all compiler and linker settings shared over all linux_x64 configurations for
    the 'debug' configuration
    """
    v = conf.env
    load_linux_x64_common_settings(v)
    
@conf
def load_profile_linux_x64_settings(conf):
    """
    Setup all compiler and linker settings shared over all linux_x64 configurations for
    the 'profile' configuration
    """
    v = conf.env
    load_linux_x64_common_settings(v)
    
@conf
def load_performance_linux_x64_settings(conf):
    """
    Setup all compiler and linker settings shared over all linux_x64 configurations for
    the 'performance' configuration
    """
    v = conf.env
    load_linux_x64_common_settings(v)
    
@conf
def load_release_linux_x64_settings(conf):
    """
    Setup all compiler and linker settings shared over all linux_x64 configurations for
    the 'release' configuration
    """
    v = conf.env
    load_linux_x64_common_settings(v)
    