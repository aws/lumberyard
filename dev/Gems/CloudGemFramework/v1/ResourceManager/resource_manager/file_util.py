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
# $Revision$

import fnmatch
import os
import tempfile
import shutil

from zipfile import ZipFile, ZipInfo

from errors import HandledError

def zip_directory(context, directory_path, aggregated_directories = None, aggregated_content = None):
    '''Creates a zip file containing the contents of the specified directory.

    Args:

        context: A Context object.

        directory_path: The path to the directory to zip.

        aggregated_directories (named): a dictionary where the values are additional
        directories whose contents will be included in the zip file, as either a single
        directory path string or a list of directory path strings. The keys are
        the path in the zip file where the content is put. By default no additional
        directories are included.

        aggregated_content (named): a dictionary where the values are additional content
        to include in the zip file. The keys are the path in the zip file where the
        content is put. By default no additional content is included.

    Returns:

        The full path to the created zip file.
         
    Note:

        The directory may contain a .ignore file. If it does, the file should contain
        the names of the files in the directory that will not be included in the zip. 

    '''

    with tempfile.NamedTemporaryFile(
        prefix=os.path.basename(directory_path) + '_', 
        suffix='.zip', 
        dir=os.path.dirname(directory_path), 
        delete=False) as file:
            zip_file_path = file.name

    try:

        with ZipFile(zip_file_path, 'w') as zip_file:
                
            __zip_directory(context, zip_file, zip_file_path, directory_path)

            if aggregated_directories:
                for destination_path, directory_paths in aggregated_directories.iteritems():
                    if not isinstance(directory_paths, list):
                        directory_paths= [ directory_paths ]
                    for directory_path in directory_paths:
                        __zip_directory(context, zip_file, zip_file_path, directory_path, destination_path)

            if aggregated_content:
                for destination_path, content in aggregated_content.iteritems():
                    __zip_content(context, zip_file, zip_file_path, destination_path, content)

        return zip_file_path

    except:
        if os.path.exists(zip_file_path):
            context.view.deleting_file(zip_file_path)
            os.remove(zip_file_path)
        raise


def __zip_content(context, zip_file, zip_file_path, destination_path, content):

    context.view.add_zip_content(destination_path, zip_file_path)

    try:

        info = ZipInfo(destination_path)
        info.external_attr = 0777 << 16L # give full access to included file

        zip_file.writestr(info, content)

    except Exception as e:
        raise HandledError('Could not add content to zip file: {}'.format(e.message))


def __zip_directory(context, zip_file, zip_file_path, src_directory_path, dst_directory_path = None):

    context.view.add_zip_content(src_directory_path, zip_file_path)

    should_ignore = create_ignore_filter_function(context, src_directory_path, '.ignore', always_ignore = ['*.pyproj', '*.pyc', '.import'])

    for src_root, src_directory_names, src_file_names in os.walk(src_directory_path):
        for src_file_name in src_file_names:
            src_file_path = os.path.join(src_root, src_file_name)
            dst_file_path = os.path.relpath(src_file_path, src_directory_path)
            if dst_directory_path is not None:
                dst_file_path = os.path.join(dst_directory_path, dst_file_path)
            if not should_ignore(src_file_path):
                try:
                    zip_file.write(src_file_path, dst_file_path)
                except Exception as e:
                    raise HandledError('Could not add {} to zip file: {}'.format(src_file_path, e.message))


def copy_directory_content(context, destination_path, source_path, overwrite_existing = False, name_substituions = {}, content_substitutions = {}):
    '''Copy the contents of a directory. The source directory may contain an 
    .ignore_when_copying file listing content that should not be copied.

    Arguments

        destination_path: the directory to which the content is copied.

        source_path: the directory from which the content is copied.

        overwrite_existing (default = False): indicates if content in the
        destination directory will be overwritten.

        name_substitutions (default = {}): dict used to replace parts 
        of file and directory names when doing the copy. For each key, 
        value in the dict all occurences of key will be replaced by value 
        in the destination file name.

        content_substitutions (default = {}): dict used to replace 
        content in copied files. The keys are the content to look for in 
        the file and the value is the content that will replace the key.
        Note that this processing is done for each and every file, don't
        use if your copying a directory with lots of files in it.

    '''

    if not os.path.isdir(source_path):
        raise HandledError('Source is not a directory when copying from {} to {}.'.format(source_path, destination_path))

    if not os.path.exists(destination_path):
        context.view.creating_directory(destination_path)
        os.makedirs(destination_path)

    if not os.path.isdir(destination_path):
        raise HandledError('Destination is not a directory when copying from {} to {}. '.format(source_path, destination_path))

    should_ignore = create_ignore_filter_function(context, source_path, '.ignore_when_copying')

    for root, dirs, files in os.walk(source_path):

        if should_ignore(root):
            continue

        relative_path = os.path.relpath(root, source_path)
        if relative_path == '.':
            dst_directory = destination_path
        else:
            dst_directory = os.path.join(destination_path, relative_path)
            if not os.path.exists(dst_directory):
                context.view.creating_directory(dst_directory)
                os.mkdir(dst_directory)

        for src_name in files:
            
            src_file = os.path.join(root, src_name)
            if should_ignore(src_file):
                continue

            dst_name = src_name
            for key, value in name_substituions.iteritems():
                dst_name = dst_name.replace(key, value)

            dst_file = os.path.join(dst_directory, dst_name)

            copy_file(context, dst_file, src_file, overwrite_existing, content_substitutions)


def copy_file(context, destination_path, source_path, overwrite_existing = False, content_substitutions = {}):                
    '''Copies a file.

    Arguments:

        context: A Context object.

        destination_path: the destination file path.

        source_path: the source file path.

        overwrite_existing (default = False): set to True to overwrite
        existing files.

        content_substitutions (default = {}): dict used to replace 
        content in copied files. The keys are the content to look for in 
        the file and the value is the content that will replace the key.
    '''

    if not os.path.isfile(source_path):
        raise HandledError('Source is not a file when copying from {} to {}.'.format(source_path, destination_path))

    if os.path.isdir(destination_path):
        raise HandledError('Destination is a directory when copying from {} to {}.'.format(source_path, destination_path))

    if not overwrite_existing and os.path.exists(destination_path):
        context.view.file_exists(destination_path)
    elif os.path.exists(destination_path) and not os.access(destination_path, os.W_OK):
        raise HandledError('Destination is not writable when copying from {} to {}.'.format(source_path, destination_path))
    else:
        context.view.copying_file(source_path, destination_path)
        shutil.copyfile(source_path, destination_path)

    if content_substitutions:

        with open(destination_path, 'r') as file:
            content = file.read()

        for key, value in content_substitutions.iteritems():
            content = content.replace(key, value)

        with open(destination_path, 'w') as file:
            file.write(content)


def create_file(context, destination_path, initial_content, overwrite_existing = False):
    '''Creates a file with some initial content.

    Args:

        destination_path: the path and name of the file.

        initial_content: The file's initial content.

        overwrite_existing (False): Overwite existing files. 

    Returns:

        True if the file was created.
    '''

    if os.path.exists(destination_path) and not overwrite_existing:
        context.view.file_exists(destination_path)
        return False
    elif os.path.exists(destination_path) and not os.access(destination_path, os.W_OK):
        raise HandledError('Destination is not writable when creating {}.'.format(destination_path))
    else:
        context.view.creating_file(destination_path)
        with open(destination_path, 'w') as file:
            file.write(initial_content)
        return True


def create_ignore_filter_function(context, ignore_file_path, ignore_file_name, always_ignore = []):
    ignore_filters = []

    try:
        ignore_file = open( os.path.join(ignore_file_path, ignore_file_name), 'rt')
        ignore_filters = ignore_file.readlines()
        ignore_file.close()
    except IOError:
        pass

    # Always ignore the ignore file
    ignore_filters.extend(always_ignore)
    ignore_filters.append(ignore_file_name)

    # Go through each ignore filter, normalizing them (fnmatch doesn't handle up level references well), and handling any special cases
    for i in range(len(ignore_filters)):

        # Get rid of any white space, ignore blank lines
        ignore_filter = ignore_filters[i].strip()
        if not ignore_filter:
            continue

        # Do not allow absolute paths to be used
        if os.path.isabs(ignore_filter):
            print 'Error, ignore filter \'' + ignore_filter + '\' is an absolute path and will not be used'
            continue

        # Cocatenate the ignore file path to get the ignore filter in the same 'space'
        ignore_filter = os.path.join(ignore_file_path, ignore_filter)

        # Normalize the path, removing any up/current level references, etc..
        ignore_filter = os.path.normpath(ignore_filter)

        # If this is a directory, add an asterisk so that we ignore all files in that directory
        if os.path.isdir( os.path.abspath(ignore_filter) ):
            ignore_filter = os.path.join(ignore_filter, '*')

        ignore_filters[i] = ignore_filter

    return lambda file_name: __is_file_ignored(file_name, ignore_filters)

def __is_file_ignored(filename, ignore_filters):
    for filter in ignore_filters:
        if fnmatch.fnmatch(filename, filter):
            return True
    return False

