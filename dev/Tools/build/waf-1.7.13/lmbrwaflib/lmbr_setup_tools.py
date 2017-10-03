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

from waflib.Configure import conf

@conf
def get_lmbr_setup_tools_output_folder(ctx, platform_override=None, configuration_override=None):
    curr_platform, curr_configuration = ctx.get_platform_and_configuration()

    if platform_override is not None and isinstance(platform_override, basestring):
        curr_platform = platform_override

    if configuration_override is not None and isinstance(configuration_override, basestring):
        curr_configuration = configuration_override

    output_folder_platform      = ""
    output_folder_compiler      = ""
    if curr_platform.startswith("win_"):
        output_folder_platform  = "Win"
        if "vs2013" in curr_platform:
            output_folder_compiler = "vc120"
        elif "vs2015" in curr_platform:
            output_folder_compiler = "vc140"

    elif curr_platform.startswith("darwin_"):
        output_folder_platform  = "Mac"
        output_folder_compiler  = "clang"
    elif curr_platform.startswith("linux_"):
        output_folder_platform  = "Linux"
        output_folder_compiler  = "clang"

    output_folder_configuration = ""
    if curr_configuration == "debug":
        output_folder_configuration = "Debug"
    elif curr_configuration == "profile":
        output_folder_configuration = "Profile"

    output_folder = "Tools/LmbrSetup/" + output_folder_platform

    # do not manipulate string if we do not have all the data
    if output_folder_platform != "" and output_folder_compiler != "" and output_folder_configuration != "":
        if output_folder_configuration != "Profile":
            output_folder += "." + output_folder_compiler
            output_folder += "." + output_folder_configuration
        elif output_folder_compiler != "vc140" and output_folder_compiler != "clang":
            output_folder += "." + output_folder_compiler

    return output_folder
