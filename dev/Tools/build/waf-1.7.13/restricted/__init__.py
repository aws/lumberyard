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

# System Imports
from common import utils
import os
import pkgutil
import sys
import importlib


def run(conf):
    #call run on all restricted packages
    dirname = os.path.dirname(os.path.abspath(__file__))
    for importer, package_name, _ in pkgutil.iter_modules([dirname]):
        
        full_package_name = '%s.%s' % (dirname, package_name)
        
        # Perform some housekeeping on any stale .pyc files that may be in the package folder
        package_folder_path = os.path.join(dirname, package_name)
        utils.clean_stale_pycs(package_folder_path)
        
        # After housecleaning of pyc files, now make sure the '__init__.py' exists in the package subfolder
        # (it may have been cleaned up by the pyc housekeeping) before we even attemp to load the package
        has_init_py = os.path.isfile(os.path.join(package_folder_path, '__init__.py'))

        if has_init_py and full_package_name not in sys.modules:
            
            module_spec = importer.find_spec(package_name)

            module = importlib.util.module_from_spec(module_spec)

            module_spec.loader.exec_module(module)

            module.run(conf)
