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
import os
import json

from waflib.Build import BuildContext
from waflib import Context, TaskGen, Logs
from waflib.Task import Task
from waflib.Configure import conf

import lumberyard_sdks


#############################################################################
class dependency_file_generator(BuildContext):
    ''' Util class for the Integration Toolkit.  Will generate a JSON for a module inside of BinTemp/module_dependencies containing the waf_files, wscript, and code files required to build the module '''
    cmd = 'generate_module_dependency_files'
    fun = 'build'


def get_module_dependency_folder(ctx):
    return ctx.get_bintemp_folder_node().make_node('module_dependencies')


@TaskGen.taskgen_method
@TaskGen.feature('generate_module_dependency_files')
def gen_create_dependency_file(tgen):
    """
    Generate BinTemp/module_dependencies
    """

    # Make sure the use_map folder exists
    dependency_map_file_folder = get_module_dependency_folder(tgen.bld)
    dependency_map_file_folder.mkdir()

    # Get the use module list from the processing of the target's wscript file
    use_module_list = getattr(tgen, 'use_module_list')
    platform_list = getattr(tgen, 'platform_list')
    config_list = getattr(tgen, 'configuration_list')
    waf_files = getattr(tgen, 'waf_files')

    module_files_dictionary = {}
    Logs.debug("Module WAF_Files: {}".format(tgen.target))
    module_files_dictionary['target'] = tgen.target
    module_files_dictionary['target_file_path'] = tgen.path.abspath()

    module_files_dictionary['waf_files'] = {}

    for waf_file in waf_files:
        Logs.debug("{}{}".format(' '*4, waf_file.name))

        file_path = waf_file.abspath()
        module_files_dictionary['waf_files'][file_path] = []

        with open(file_path) as data_file:
            data = json.load(data_file)
            for category in data:
                for sub_category in data[category]:
                    for file in data[category][sub_category]:
                        module_files_dictionary['waf_files'][file_path].append(file)

        # Encode JSON data
        module_dependencies_directory = os.path.join(dependency_map_file_folder.parent.cache_abspath,
                                                     'module_dependencies')
        if not os.path.exists(module_dependencies_directory):
            os.makedirs(module_dependencies_directory)

        file_name = '{}_file_dependencies.json'.format(tgen.target)
        dependency_file_path = os.path.join(module_dependencies_directory, file_name)

        with open(dependency_file_path, 'w') as f:
            json.dump(module_files_dictionary, f, sort_keys=True, indent=4, separators=(',', ': '))


