from common import utils
import os
import pkgutil
import sys


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
            module = importer.find_module(package_name).load_module(full_package_name)
            module.run(conf)