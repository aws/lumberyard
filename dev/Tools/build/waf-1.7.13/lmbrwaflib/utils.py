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
import re
import os
import json
import shutil
import stat
import hashlib
import fnmatch
import datetime


def fast_copyfile(src, dst, buffer_size=1024*1024):
    """
    Copy data from src to dst - reimplemented with a buffer size
    Note that this function is simply a copy of the function from the official python shutils.py file, but
    with an increased (configurable) buffer size fed into copyfileobj instead of the original 16kb one
    """
    if shutil._samefile(src, dst):
        raise shutil.Error("`%s` and `%s` are the same file" % (src, dst))

    for fn in [src, dst]:
        try:
            st = os.stat(fn)
        except OSError:
            # File most likely does not exist
            pass
        else:
            # XXX What about other special files? (sockets, devices...)
            if stat.S_ISFIFO(st.st_mode):
                raise shutil.SpecialFileError("`%s` is a named pipe" % fn)

    with open(src, 'rb') as fsrc:
        with open(dst, 'wb') as fdst:
            shutil.copyfileobj(fsrc, fdst, buffer_size)

def fast_copy2(src, dst, buffer_size=1024*1024):
    """
    Copy data and all stat info ("cp -p src dst").
    The destination may be a directory.
    Note that this is just a copy of the copy2 function from shutil.py that calls the above fast copyfile
    """

    if os.path.isdir(dst):
        dst = os.path.join(dst, os.path.basename(src))
    fast_copyfile(src, dst, buffer_size)
    shutil.copystat(src, dst)

MAX_TIMESTAMP_DELTA_MILS = datetime.timedelta(milliseconds=1)


def should_overwrite_file(source_path, dest_path):
    """
    Generalized decision to perform a copy/overwrite of an existing file.

    For improved performance only a comparison of size and mod times of the file is checked.

    :param source_path:     The source file path to copy from
    :param dest_path:     The target file path to copy to
    :return:  True if the target file does not exist or the target file is different from the source based on a specific criteria
    """

    copy = False
    if os.path.exists(source_path):
        if not os.path.exists(dest_path):
            # File does not exist, so always perform a copy
            copy = True
        elif os.path.isfile(source_path) and os.path.isfile(dest_path):
            # Only overwrite for files
            source_stat = os.stat(source_path)
            dest_stat = os.stat(dest_path)
            # Right now, ony compare the file sizes and mod times to detect if an overwrite is necessary
            if source_stat.st_size != dest_stat.st_size:
                copy = True
            else:
                # If the sizes are the same, then check the timestamps for the last mod time
                source_mod_time = datetime.datetime.fromtimestamp(source_stat.st_mtime)
                dest_mod_time = datetime.datetime.fromtimestamp(dest_stat.st_mtime)
                if abs(source_mod_time-dest_mod_time) > MAX_TIMESTAMP_DELTA_MILS:
                    copy = True

    return copy


def copy_and_modify(src_file, dst_file, pattern_and_replace_tuples):
    """
    Read a source file, apply replacements, and save to a target file

    :param src_file:    The source file to read and do the replacements
    :param dst_file:    The target file
    :param pattern_and_replace_tuples:  Pattern+Replacement tuples
    """
    if not os.path.exists(src_file):
        raise ValueError('Source file ({}) does not exist.'.format(src_file))

    with open(src_file,'r') as src_file_file:
        src_file_content = src_file_file.read()

    for pattern_str, replacement_str in pattern_and_replace_tuples:
        src_file_content = re.sub(pattern_str, replacement_str, src_file_content)

    with open(dst_file,'w') as dst_file:
        dst_file.write(src_file_content)


def calculate_file_hash(file_path):
    """
    Quickly compute a hash (md5) of a file
    :param file_path:
    :return:
    """
    with open(file_path) as file_to_hash:
        file_content = file_to_hash.read()
    digest = hashlib.md5()
    digest.update(file_content)
    hash_result = digest.hexdigest()
    return hash_result


def copy_file_if_needed(source_path, dest_path):
    """
    Determine if we need to overwrite a file based on:
        1. File size
        2. File Hash
    :param source_path: source file path
    :param dest_path:   destination file path
    :return: True if the files are different and a copy is needed, otherwise False
    """
    copy = False
    if os.path.exists(source_path):
        if not os.path.exists(dest_path):
            # File does not exist, so always perform a copy
            copy = True
        elif os.path.isfile(source_path) and os.path.isfile(dest_path):
            # Only overwrite for files
            source_stat = os.stat(source_path)
            dest_stat = os.stat(dest_path)
            # Right now, ony compare the file sizes and mod times to detect if an overwrite is necessary
            if source_stat.st_size != dest_stat.st_size:
                copy = True
            else:
                source_hash = calculate_file_hash(source_path)
                dest_hash = calculate_file_hash(dest_path)
                copy = source_hash != dest_hash

    if copy:
        fast_copy2(source_path, dest_path)

    return copy


def copy_folder(src_folders, dst_folder, ignore_paths, status_format):
    """
    Copy a folder, overwriting existing files only if needed
    :param src_folders:  List of source folders to copy
    :param dst_folder:  The target folder destination
    :param ignore_paths:    list of items inside the folders to ignore
    :return:
    """

    def _copy_folder(src, dst, ignore):
        if not os.path.exists(src):
            return
        if not os.path.isdir(src):
            return
        if not os.path.exists(dst):
            os.makedirs(dst)

        for item in os.listdir(src):
            src_item = os.path.join(src,item)

            ignore_item = False
            for ignore_pattern in ignore:
                if fnmatch.fnmatch(src_item,ignore_pattern):
                    ignore_item = True
                    break
            if ignore_item:
                continue

            dst_item = os.path.join(dst,item)
            if os.path.isdir(src_item):
                _copy_folder(src_item,dst_item, ignore_paths)
            else:
                copy_file_if_needed(src_item, dst_item)
        return

    ignore_paths_normalized = [ os.path.normpath(os.path.abspath(ignore_path)) for ignore_path in ignore_paths]
    src_folders_normalized = [ os.path.normpath(src_folder) for src_folder in src_folders]
    for src_folder_normalized in src_folders_normalized:

        source_folder = os.path.abspath(src_folder_normalized)
        if source_folder in ignore_paths_normalized:
            continue
        target_folder = os.path.abspath(os.path.normpath(os.path.join(dst_folder, src_folder_normalized)))
        if status_format:
            print(status_format.format(src_folder_normalized))
        _copy_folder(source_folder, target_folder, ignore_paths)


def copy_files_to_folder(required_src_files, optional_src_files, dst_folder, status_format=None):
    """
    Copy files to a destination folder

    :param required_src_files:  List of required files relative to the current path
    :param optional_src_files:  List of optional files relative to the current path
    :param dst_folder:          Target folder
    :param status_format:       Optional format string to print the status of each copy
    """
    if not os.path.exists(dst_folder):
        os.makedirs(dst_folder)

    for src_file in required_src_files+optional_src_files:
        if not os.path.isfile(src_file):
            raise ValueError('"{}" is not a file.'.format(src_file))
        if not os.path.exists(src_file):
            if src_file in required_src_files:
                raise ValueError('Required file ({}) does not exist.'.format(src_file))
            else:
                continue
        source_file_abs_path = os.path.abspath(src_file)
        dest_file_abs_path = os.path.join(os.path.abspath(dst_folder), os.path.basename(src_file))
        if copy_file_if_needed(source_file_abs_path, dest_file_abs_path):
            if status_format:
                print(status_format.format(src_file))



def parse_json_file(json_file_path):
    """
    Parse a json file into a utf-8 encoded python dictionary
    :param json_file_path:
    :return:
    """

    def _decode_list(list_data):
        rv = []
        for item in list_data:
            if isinstance(item, unicode):
                item = item.encode('utf-8')
            elif isinstance(item, list):
                item = _decode_list(item)
            elif isinstance(item, dict):
                item = _decode_dict(item)
            rv.append(item)
        return rv

    def _decode_dict(dict_data):
        rv = {}
        for key, value in dict_data.iteritems():
            if isinstance(key, unicode):
                key = key.encode('utf-8')
            if isinstance(value, unicode):
                value = value.encode('utf-8')
            elif isinstance(value, list):
                value = _decode_list(value)
            elif isinstance(value, dict):
                value = _decode_dict(value)
            rv[key] = value
        return rv

    try:
        with open(json_file_path) as json_file:
            json_file_content = json_file.read()

        json_file_data = json.loads(json_file_content, object_hook=_decode_dict)
        return json_file_data
    except Exception as e:
        raise ValueError('Error reading {}: {}'.format(json_file_path, e.message))

def write_json_file(json_file_data, json_file_path):
    """

    :param json_file_data:
    :param json_file_path:
    :return:
    """
    try:
        with open(json_file_path,"w") as json_file:
            json_file_content = json.dumps(json_file_data,sort_keys=True,separators=(',',': '),indent=4)
            json_file.write(json_file_content)
    except Exception as e:
        raise ValueError('Error writing {}: {}'.format(json_file_path, e.message))

