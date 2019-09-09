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
import os
import subprocess
# being a _host file, this means that these settings apply to any build at all that is
# being done from this kind of host

PLATFORM = 'darwin_x64'

@conf
def load_darwin_x64_host_settings(conf):
    """
    Setup any environment settings you want to apply globally any time the host doing the building is darwin (OSX) x64
    """
    v = conf.env

    global PLATFORM

    azcg_dir = conf.Path('Tools/AzCodeGenerator/bin/osx')

    v['CODE_GENERATOR_EXECUTABLE'] = 'AzCodeGenerator'
    v['CODE_GENERATOR_PATH'] = [ azcg_dir ]
    v['CODE_GENERATOR_PYTHON_PATHS'] = ['/System/Library/Frameworks/Python.framework/Versions/2.7',
                                        '/System/Library/Frameworks/Python.framework/Versions/2.7/lib',
                                        '/System/Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7',
                                        '/System/Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/lib-dynload',
                                        conf.ThirdPartyPath('markupsafe', 'x64'),
                                        conf.ThirdPartyPath('jinja2', 'x64')]
    v['CODE_GENERATOR_PYTHON_DEBUG_PATHS'] = [conf.ThirdPartyPath('markupsafe', 'x64'),
                                              conf.ThirdPartyPath('jinja2', 'x64')]

    v['CODE_GENERATOR_PYTHON_HOME'] = '/System/Library/Frameworks/Python.framework/Versions/2.7'
    v['CODE_GENERATOR_PYTHON_HOME_DEBUG'] = '/System/Library/Frameworks/Python.framework/Versions/2.7'
    v['CODE_GENERATOR_INCLUDE_PATHS'] = []

    clang_search_dirs = subprocess.check_output(['clang++', '-print-search-dirs']).strip().split('\n')
    clang_search_paths = {}
    for search_dir in clang_search_dirs:
        (type, dirs) = search_dir.split(': =')
        clang_search_paths[type] = dirs.split(':')
    v['CLANG_SEARCH_PATHS'] = clang_search_paths

    # Detect the QT binaries, if the current capabilities selected requires it.
    _, enabled, _, _ = conf.tp.get_third_party_path(PLATFORM, 'qt')
    if enabled:
        conf.find_qt5_binaries(PLATFORM)
