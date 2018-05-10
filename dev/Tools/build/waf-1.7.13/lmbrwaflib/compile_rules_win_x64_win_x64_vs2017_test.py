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

import compile_rules_win_x64_win_x64_vs2017

def load_common_win_x64_vs2017_test_settings(conf):
    conf.env['DEFINES'] += ['AZ_TESTS_ENABLED']

    azcg_dir = conf.srcnode.make_node('Tools/AzCodeGenerator/bin/vc141').abspath()
    if not os.path.exists(azcg_dir):
        conf.fatal('Unable to locate the AzCodeGenerator subfolder.  Make sure that you have VS2015 AzCodeGenerator binaries available')


@conf
def load_debug_win_x64_win_x64_vs2017_test_settings(conf):
    load_common_win_x64_vs2017_test_settings(conf)
    return compile_rules_win_x64_win_x64_vs2017.load_debug_win_x64_win_x64_vs2017_settings(conf)

@conf
def load_profile_win_x64_win_x64_vs2017_test_settings(conf):
    load_common_win_x64_vs2017_test_settings(conf)
    return compile_rules_win_x64_win_x64_vs2017.load_profile_win_x64_win_x64_vs2017_settings(conf)

