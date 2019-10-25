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

########################################################################################################################
# Global constants
########################################################################################################################
BINTEMP_FOLDER = 'BinTemp'             # Name of the of working build folder that will be created at the root folder
WAF_FILE_GLOB_WARNING_THRESHOLD = 1000 # Define a warning threshold in number file files that were hit during a waf_file
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
LMBR_WAF_VERSION_TAG = "E7A426A7-350D-4B4A-A619-ED1DA4463DCA"


# Optional additional table of copyrights.
# To add a company specific copyright, add a name value pair below to define the desired copyright statement for
# generated binaries and add the 'copyright_org' in your wscript definition
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
########################################################################################################################
ADDITIONAL_COPYRIGHT_TABLE = {
}



########################################################################################################################
# Optional table of additional modules that will be loaded by WAF
#
# The table format is:
#
# <Key: Path of the directory for a set of waf modules> :
#    [ < List of WAF python modules to load into the WAF build system, relative to the path directory from the key > ]
#
# For each of the modules in the python module list, they represent the relative full filename of the module to load.
# To restrict modules to only load for a specific host platform
#
# e.g.
#
# ADDITIONAL_WAF_MODULES = {
#    'Tools/Build/custom_build' : [
#        'custom_a.py',
#        'custom_b.py:win32',
#        'custom_c.py:darwin'
#    ]
# }
#
# The above example will load 'custom_a.py' for all platforms, 'custom_b.py' for only win32 platforms, and 'custom_c.py'
# for only darwin platforms
#
# Note:  The methods that are to exposed in the modules must be decorated accordingly, as they are generally used
#        based on the context of the command, and not through regular python imports
########################################################################################################################
ADDITIONAL_WAF_MODULES = {

}

########################################################################################################################
# Lumberyard version and build number information.
# The following section extrapolates the version number from the engine configuration file and is used to embed the
# value into the built binaries were applicable.
########################################################################################################################
LUMBERYARD_ENGINE_VERSION_CONFIG_FILENAME = 'engine.json'
SCRIPT_PATH = os.path.dirname(__file__)

with open(os.path.join(SCRIPT_PATH, LUMBERYARD_ENGINE_VERSION_CONFIG_FILENAME)) as ENGINE_FILE:
    ENGINE_JSON_DATA = json.load(ENGINE_FILE)

LUMBERYARD_VERSION = ENGINE_JSON_DATA.get('LumberyardVersion', '0.0.0.0').encode("ascii", "ignore")
LUMBERYARD_COPYRIGHT_YEAR = ENGINE_JSON_DATA.get('LumberyardCopyrightYear', 2017)
LUMBERYARD_BUILD = 979871
LUMBERYARD_ENGINE_PATH = os.path.normpath(ENGINE_JSON_DATA.get('ExternalEnginePath', '.').encode("ascii", "ignore"))

# validate the Lumberyard version string above
VERSION_NUMBER_PATTERN = re.compile("^(\.?\d+)*$")
if VERSION_NUMBER_PATTERN.match(LUMBERYARD_VERSION) is None:
    raise ValueError('Invalid version string for the Lumberyard Version ({})'.format(LUMBERYARD_VERSION))

BINTEMP_CACHE_3RD_PARTY = '__cache_3p__'
BINTEMP_CACHE_TOOLS = '__cache_tools_'
BINTEMP_MODULE_DEF = 'module_def'

########################################################################################################################
# Sub folder within the root folder to recurse into for the WAF build.
########################################################################################################################
SUBFOLDERS = [
    'Code',
    'Engine',
]


########################################################################################################################
# List of available launchers by spec module
########################################################################################################################
AVAILABLE_LAUNCHERS = {
    'modules':
        [
            'WindowsLauncher',
            'MacLauncher',
            'DedicatedLauncher',
            'IOSLauncher',
            'AppleTVLauncher',
            'AndroidLauncher',
            'LinuxLauncher',

            'ClientLauncher',
            'ServerLauncher'
        ]
    }
