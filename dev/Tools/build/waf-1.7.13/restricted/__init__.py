import os
import pkgutil
import sys

def run(conf):
    #call run on all restricted packages
    dirname = os.path.dirname(os.path.abspath(__file__))
    for importer, package_name, _ in pkgutil.iter_modules([dirname]):
        full_package_name = '%s.%s' % (dirname, package_name)
        if full_package_name not in sys.modules:
            module = importer.find_module(package_name).load_module(full_package_name)
            module.run(conf)