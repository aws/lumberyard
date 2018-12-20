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
CACHE_FOLDER = 'Cache'


# Version stamp (GUID) of lmbrwaf that is used to signal that a clean of bintemp is necessary
# Only update this number if there are changes in WAF handling where it is not possible
# to track stale intermediate files caused by the waf changes.  To ignore the bintemp
# cleaning check, set this value to None.
#
# Note:  Only update this value as a last resort.  If there were WAF changes that do not affect the generation or
#        tracking of intermediate of generated files, then there is no need to wipe out BinTemp
LMBR_WAF_VERSION_TAG = "AE99E23F-9EE1-410C-846F-0E79A82DFC39"   # Change for replacing IDX values with deterministic numbers


BUILD_ENV_NAME = 'UNDEFINED'


import json
import os

if(os.path.exists('BuildEnv.json')):
    with open('BuildEnv.json') as jsonFile:
        data = json.load(jsonFile)
        BUILD_ENV_NAME = data["env"]["name"]
else:
    print('A custom BuildEnv.json file was not found. This build will be untagged.')



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
LUMBERYARD_COPYRIGHT_YEAR = ENGINE_JSON_DATA.get('LumberyardCopyrightYear', 2017)
LUMBERYARD_BUILD = 785010
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
        'win_x64_clang',
        'win_x64_vs2017',
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
    'non_dedicated': ['debug', 'debug_test', 'profile', 'profile_test', 'performance', 'release'],
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
non_test_platforms = (
    'android_armv7_gcc',
    'android_armv7_clang',
    'android_armv8_clang',
    'appletv',
    'ios',
)

non_dedicated_platforms = (
    'android_armv7_gcc',
    'android_armv7_clang',
    'android_armv8_clang',
    'appletv',
    'darwin_x64',
    'ios',
)

platforms_to_filter = set(non_test_platforms + non_dedicated_platforms)

PLATFORM_CONFIGURATION_FILTER = { }
for platform in platforms_to_filter:

    config_key_1 = 'non_test' if platform in non_test_platforms else 'all'
    config_key_2 = 'non_dedicated' if platform in non_dedicated_platforms else 'all'

    config_set_1 = CONFIGURATION_SHORTCUT_ALIASES[config_key_1]
    config_set_2 = CONFIGURATION_SHORTCUT_ALIASES[config_key_2]

    PLATFORM_CONFIGURATION_FILTER[platform] = list(set(config_set_1).intersection(config_set_2))

## what conditions do you want a monolithic build ?  Uses the same matching rules as other settings
## so it can be platform_configuration, or configuration, or just platform for the keys, and the Value is assumed
## false by default.
## monolithic builds produce just a statically linked executable with no dlls.

MONOLITHIC_BUILDS = [
    'win_x64_clang_release',
    'win_x64_vs2017_release',
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
