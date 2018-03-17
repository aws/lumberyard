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
# $Revision: #2 $

import os

from resource_manager_common import constant

from errors import HandledError

def resolve_imports(context, target_directory_path, imported_paths = None, multi_imports = None):
    '''Determines the paths identified by an .import file in a specified path.

    The .import file should contain one import name per line. An import name has
    the format {gem-name}.{common-code-subdirectory-name}.

    If the gem name is * then the common code directory will be imported from
    any and all active gems that provide it.

    Imports are resolved recursivly in an arbitary order. 
    
    TODO: is this good enough, or does this need to be ordered? What about duplicate 
    content? Should it be detected here? Is it always an error or, if the paths are 
    ordered, is precidence assumed. Starting with the fewest assumptions and we'll 
    see what problems manifest.

    Arguments:

        context: A Context object.

        target_directory_path: the path that may or may not contain an .import file.

        imported_paths (optional): a set of paths that have already been imported.

        multi_imports (optional): a dictionary of import name to a set of gem names
            participating in the import.

    Returns two values:
   
        imported_paths: a set of full paths to imported directories.
            All the directories will have been verified to exist.

        multi_imports: a dictionary of import name to a set of gem names participating
            in the import.  All the directories will have been verified to exist.

    Raises a HandledError if the .import file contents are malformed, identify a gem
    that does not exist or is not enabled, or identifies an common-code directory 
    that does not exist.

    '''

    if imported_paths is None:
        imported_paths = set()

    if multi_imports is None:
        multi_imports = {}

    imports_file_path = os.path.join(target_directory_path, constant.IMPORT_FILE_NAME)
    if os.path.isfile(imports_file_path):

        with open(imports_file_path, 'r') as imports_file:
            lines = imports_file.readlines()

        for line in lines:

            # parse the line
            line = line.strip()
            parts = line.split('.')
            if len(parts) != 2 or not parts[0] or not parts[1]:
                raise HandledError('Invalid import "{}" found in {}. Import should have the format <gem-name>.<common-code-subdirectory-name>.'.format(line, imports_file_path))

            gem_name, import_directory_name = parts

            if gem_name == '*':

                imported_gem_names = multi_imports.get(import_directory_name)
                if imported_gem_names is None:
                    imported_gem_names = set()
                    multi_imports[import_directory_name] = imported_gem_names

                # search all enabled gems for directories to import

                for gem in context.gem.enabled_gems:
                    # check if the common-code subdirectory exists

                    path = os.path.join(gem.aws_directory_path, constant.COMMON_CODE_DIRECTORY_NAME, import_directory_name)
                    if not os.path.isdir(path):
                        continue

                    # add it to the set and recurse

                    if path not in imported_paths: # avoid infinte loops and extra work
                        imported_paths.add(path)
                        imported_gem_names.add(gem.name)
                        resolve_imports(context, path, imported_paths, multi_imports)
            else:
                # get the identified gem

                gem = context.gem.get_by_name(gem_name)
                if gem is None:
                    raise HandledError('The {} gem does not exist but is required by the {} import in {}.'.format(gem_name, line, imports_file_path))
                if not gem.is_enabled:
                    raise HandledError('The {} gem is not enabled but is required by the {} import in {}.'.format(gem_name, line, imports_file_path))

                # get the identified common-code subdirectory

                path = os.path.join(gem.aws_directory_path, constant.COMMON_CODE_DIRECTORY_NAME, import_directory_name)
                if not os.path.isdir(path):
                    raise HandledError('The {} gem does not provide the {} import as found in {}. The {} directory does not exist.'.format(gem_name, line, imports_file_path, path))

                # add it to the set and recurse

                if path not in imported_paths: # avoid infinte loops and extra work
                    imported_paths.add(path)
                    resolve_imports(context, path, imported_paths, multi_imports)

    return imported_paths, multi_imports
