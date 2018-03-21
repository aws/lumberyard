########################################################################################
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
########################################################################################

# Central configuration file for a branch, should never be integrated since it is unique for each branch
import json
import os
import re

######################
## Build Layout
BINTEMP_FOLDER = 'BinTemp'
SOLUTION_FOLDER = 'Solutions'
SOLUTION_NAME = 'Lumberyard'


# Optional additional table of copyrights.
# To add a company specific copyright, add a name value pair below to define the desired copyright statement for generated binaries
# and add the 'copyright_org' in your wscript definition
#
# e.g.
#
# ADDITIONAL_COPYRIGHT_TABLE = {
#     'MyCompany' : 'Copyright (c) MyCompany'
# }
#
# and in your module definition
#
# bld.CryEngineModule(
#  ...
#    copyright_org = 'MyCompany'
# ...
# )
ADDITIONAL_COPYRIGHT_TABLE = {
}


# Lumberyard version and build number information.  This value will be embedded outputs for the lumberyard build
LUMBERYARD_ENGINE_VERSION_CONFIG_FILENAME = 'engine.json'
SCRIPT_PATH = os.path.dirname(__file__)

with open(os.path.join(SCRIPT_PATH, LUMBERYARD_ENGINE_VERSION_CONFIG_FILENAME)) as ENGINE_FILE:
    ENGINE_JSON_DATA = json.load(ENGINE_FILE)

LUMBERYARD_VERSION = ENGINE_JSON_DATA.get('LumberyardVersion', '0.0.0.0').encode("ascii", "ignore")
LUMBERYARD_BUILD = 570429
LUMBERYARD_ENGINE_PATH = os.path.normpath(ENGINE_JSON_DATA.get('ExternalEnginePath', '.').encode("ascii", "ignore"))

# validate the Lumberyard version string above
VERSION_NUMBER_PATTERN = re.compile("^(\.?\d+)*$")
if VERSION_NUMBER_PATTERN.match(LUMBERYARD_VERSION) is None:
    raise ValueError('Invalid version string for the Lumberyard Version ({})'.format(LUMBERYARD_VERSION))

# Sub folder within the root folder to recurse into for the WAF build
SUBFOLDERS = [
    'Code',
    'Engine',
]

# Supported branch platforms/configurations This is a map of host platform -> target platforms
# Target platforms can be removed here if they are meant to never be considered.
# Caution: Do not remove the host platform.
PLATFORMS = {
    'darwin': [
        'darwin_x64',
        'android_armv7_clang',
        'android_armv8_clang',
        'ios',
        'appletv'
    ],
    'win32' : [
        'win_x64_vs2015',
        'win_x64_vs2013',
        'android_armv7_clang',
        'android_armv8_clang'
    ],
    'linux': [
        'linux_x64'
    ]
}




# list of build configurations to generate for each supported platform
CONFIGURATIONS = [ 'debug',           'profile',           'performance',           'release',
                   'debug_dedicated', 'profile_dedicated', 'performance_dedicated', 'release_dedicated',
                   'debug_test',      'profile_test',
                   'debug_test_dedicated', 'profile_test_dedicated']

# To handle configurations aliases that can map to one or more actual configurations (Above).
# Make sure to maintain and update the alias based on updates to the CONFIGURATIONS
# dictionary
CONFIGURATION_SHORTCUT_ALIASES = {
    'all': CONFIGURATIONS,
    'debug_all': ['debug', 'debug_dedicated', 'debug_test', 'debug_test_dedicated'],
    'profile_all': ['profile', 'profile_dedicated', 'profile_test', 'profile_test_dedicated'],
    'performance_all': ['performance', 'performance_dedicated'],
    'release_all': ['release', 'release_dedicated'],
    'dedicated_all': ['debug_dedicated', 'profile_dedicated', 'performance_dedicated', 'release_dedicated'],
    'non_dedicated': ['debug', 'profile', 'performance', 'release'],
    'test_all': ['debug_test', 'debug_test_dedicated', 'profile_test', 'profile_test_dedicated'],
    'non_test': ['debug', 'debug_dedicated', 'profile', 'profile_dedicated', 'performance', 'performance_dedicated',
                 'release', 'release_dedicated']
}

# Keep the aliases in sync
for configuration_alias_key in CONFIGURATION_SHORTCUT_ALIASES:
    if configuration_alias_key in CONFIGURATIONS:
        raise ValueError("Invalid configuration shortcut alias '{}' in waf_branch_spec.py. Duplicates an existing configuration.".format(configuration_alias_key))
    configuration_alias_list = CONFIGURATION_SHORTCUT_ALIASES[configuration_alias_key]
    for configuration_alias in configuration_alias_list:
        if configuration_alias not in CONFIGURATIONS:
            raise ValueError("Invalid configuration '{}' for configuration shortcut alias '{}' in waf_branch_spec.py".format(configuration_alias,configuration_alias_key))

# build/clean commands are generated using PLATFORM_CONFIGURATION for all platform/configuration
# not all platform/configuration commands are valid.
# if an entry exists in this dictionary, only the configurations listed will be built
PLATFORM_CONFIGURATION_FILTER = {
    # Remove these as testing comes online for each platform
    platform : CONFIGURATION_SHORTCUT_ALIASES['non_test'] for platform in ( 'android_armv7_gcc',
                                                                            'android_armv7_clang',
                                                                            'android_armv8_clang',
                                                                            'ios',
                                                                            'appletv')
}

## what conditions do you want a monolithic build ?  Uses the same matching rules as other settings
## so it can be platform_configuration, or configuration, or just platform for the keys, and the Value is assumed
## false by default.
## monolithic builds produce just a statically linked executable with no dlls.

MONOLITHIC_BUILDS = [
    'win_x64_vs2015_release',
    'win_x64_vs2013_release',
    'release_dedicated',
    'performance_dedicated',
    'performance',
    'ios',
    'appletv',
    'darwin_release',
    'android_release',
]

## List of available launchers by spec module
AVAILABLE_LAUNCHERS = {
    'modules':
        [
            'WindowsLauncher',
            'MacLauncher',
            'DedicatedLauncher',
            'IOSLauncher',
            'AppleTVLauncher',
            'AndroidLauncher',
            'LinuxLauncher'
        ]
    }
