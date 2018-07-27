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
from waflib.Task import Task, RUN_ME, SKIP_ME
from waflib.TaskGen import after_method, before_method, feature, extension, taskgen_method
from cry_utils import flatten_list
from utils import fast_copy2, should_overwrite_file, calculate_file_hash
from waflib import Logs, Utils, Errors
import os
import glob
import shutil
import time
import stat
import datetime

try:
    import _winreg
except ImportError:
    pass
DEPENDENT_TARGET_BLACKLISTED_SUBFOLDERS = ['EditorPlugins']

def hash_range(filename, start, size):
    f = open(filename, 'rb')
    m = Utils.md5()
    try:
        if start:
            f.seek(start)
        chunk = f.read(size)
        m.update(chunk)
    finally:
        f.close()
    return m.digest()

def fast_hash(fname):
    """
    Computes a hash value for a file by using md5 on the file name, modified
    timestamp and file size
    """
    st = os.stat(fname)
    if stat.S_ISDIR(st[stat.ST_MODE]):
        raise IOError('not a file')
    m = Utils.md5()
    m.update(str(st.st_mtime))
    m.update(str(st.st_size))
    m.update(fname)
    return m.digest()

# pdbs and dlls have a guid embedded in their header which should change
# every time they are compiled/linked
# for dylibs and files that don't have an extension (executables on macOS falls
# into this category as well as wscript files) use the fast_hash as the default
# hash is too slow for large files
HASH_OVERRIDES = {
    ".pdb": lambda filename: hash_range(filename, 0, 256),
    ".dll": lambda filename: hash_range(filename, 0, 512),
    ".dylib": fast_hash,
    "": fast_hash,
}

h_file_original = Utils.h_file

def h_file(filename):
    basename, ext = os.path.splitext(filename)
    hash = HASH_OVERRIDES.get(ext, h_file_original)
    return hash(filename)

Utils.h_file = h_file


def copy_tree2(src, dst, overwrite_existing_file=False, pattern_paths=None, is_pattern_required=False, fail_on_error=True):
    """
    Copy a tree from a source folder to a destination folder.  If the destination does not exist, then create
    a copy automatically.  If a destination does exist, then either overwrite based on the owerwrite_existing_file
    parameter or based on if the file is different (currently only file size is checked)

    :param src:     The source tree to copy from
    :param dst:     The target tree to copy to
    :param overwrite_existing_file:     Flag to always overwrite (otherwise follow the copy rule)
    :param is_pattern_required: Tells the function how to interpret the pattern_paths, pattern_paths are required
     paths if is_pattern_required is True, it will be the ignore paths if it is False
    :param pattern_paths:    Any particular file/pattern to ignore
    :return:  The number of files actually copied
    """

    if os.path.isdir(src) is False:
        Logs.warn('[WARN] Unable to copy {} to destination {} using copy_tree2. {} is not a directory.'.format(src, dst, src))
        return 0

    supports_symlinks = not Utils.unversioned_sys_platform().startswith('win')

    src = os.path.normpath(src)
    dst = os.path.normpath(dst)

    # get all non ignored paths/files in dir based on ignore path input
    def _get_non_ignored_paths_in_dir_recursively(src, ignore_paths):
        non_ignored_paths = []
        paths = os.listdir(src)

        for item in paths:
            ignore = False
            src_path = os.path.join(src, item)

            if any(path in src_path for path in ignore_paths):
                continue

            if supports_symlinks and os.path.islink(src_path):
                non_ignored_paths.append(src_path)
            elif os.path.isdir(src_path):
                non_ignored_paths.extend(_get_non_ignored_paths_in_dir_recursively(src_path, ignore_paths))
            else:
                non_ignored_paths.append(src_path)

        return non_ignored_paths

    # copy everything if pattern_path is none
    paths_to_copy = os.listdir(src)
    copied_files = 0

    if pattern_paths is not None:
        filtered_paths = []
        if is_pattern_required is True:
            for path in pattern_paths:
                filtered_paths.extend(glob.glob(os.path.join(src, path)))

        else:
            filtered_paths = _get_non_ignored_paths_in_dir_recursively(src, pattern_paths)

        # sanitize the paths in filtered_paths for further consumption (we only want relative path from src)
        for idx, item in enumerate(filtered_paths):
            item = os.path.normpath(item)
            sep = src + os.path.sep
            filtered_paths[idx] = item.replace(sep, "")

        paths_to_copy = filtered_paths

    symlinks = []
    # now we copy all files specified from the paths in paths_to_copy
    for path in paths_to_copy:
        srcname = os.path.join(src, path)
        dstname = os.path.join(dst, path)

        if supports_symlinks and os.path.islink(srcname):
            linkto = os.readlink(srcname)
            symlinks.append([linkto, dstname])

        elif os.path.isdir(srcname):
            # if we encounter a srcname that is a folder, we assume that we want the entire folder
            # pattern_paths to None tells this function that we want to copy the entire folder
            copied_files += copy_tree2(srcname, dstname, overwrite_existing_file, None, fail_on_error=fail_on_error)

        else:
            # check to see if we should copy the file
            copy = overwrite_existing_file or should_overwrite_file(srcname, dstname)
            if copy is False:
                continue
            file_copied = 0

            Logs.debug('lumberyard: Copying file {} to {}'.format(srcname, dstname))
            try:
                # In case the file is readonly, we'll remove the existing file first
                if os.path.exists(dstname):
                    os.chmod(dstname, stat.S_IWRITE)

                # In the case where the path doesn't exist
                elif os.path.exists(os.path.dirname(dstname)) is False:
                    try:
                        os.makedirs(os.path.dirname(dstname))
                    except:
                        pass
                    if not os.path.exists(dst):
                        raise shutil.Error("Unable to create target folder `{}`".format(dst))

                try:
                    file_copied = fast_copy2(srcname, dstname)
                except:
                    # Second try with detail hash check
                    file_copied = fast_copy2(srcname, dstname, True)

            except Exception as err:
                if fail_on_error:
                    raise err
                else:
                    Logs.warn(
                        '[WARN] Unable to copy {} to destination {}.  Check the file permissions or any process that may be locking it.'.format(
                            srcname, dstname))
            copied_files += file_copied

    # symlinks will be empty if a platform does not support them so no need to
    # protect against running this code
    for symlink in symlinks:
        if os.path.exists(symlink[1]):
            if os.path.islink(symlink[1]):
                os.unlink(symlink[1]) 
            elif os.path.isdir(symlink[1]):
                shutil.rmtree(symlink[1])
            else:
                os.remove(symlink[1]) 
        try:
            os.symlink(symlink[0], symlink[1])
        except OSError as oops:
            Logs.warn("Unable to create symbolic link {} because {}".format(symlink[1], oops))
            copied_files = False

    return copied_files


class copy_outputs(Task):
    """
    Class to handle copying of the final outputs into the Bin folder
    """
    color = 'YELLOW'
    optional = False
    """If True, build doesn't fail if copy fails."""

    def __init__(self, *k, **kw):
        super(copy_outputs, self).__init__(self, *k, **kw)
        self.check_timestamp_and_size = True

    def run(self):
        src = self.inputs[0].abspath()
        tgt = self.outputs[0].abspath()

        # Create output folder
        tries = 0
        dir = os.path.dirname(tgt)
        created = False
        while not created and tries < 10:
            try:
                os.makedirs(dir)
                created = True
            except OSError as ex:
                self.err_msg = "%s\nCould not mkdir for copy %s -> %s" % (str(ex), src, tgt)
                # ignore Directory already exists when this is called from multiple threads simultaneously
                if os.path.isdir(dir):
                    created = True
            tries += 1

        if not created:
            return -1

        tries = 0
        result = -1
        while result == -1 and tries < 10:
            try:
                fast_copy2(src, tgt)
                result = 0
                self.err_msg = None
            except Exception as why:

                # If copy failed, maybe due to a permission issue, run an deeper check and compare the file's
                # finger prints, because the default check will compare size and timestamp only
                if os.path.exists(src) and os.path.exists(tgt):
                    src_hash = calculate_file_hash(src)
                    tgt_hash = calculate_file_hash(tgt)
                    if src_hash == tgt_hash:
                        result = 0
                        break

                self.err_msg = "Could not perform copy %s -> %s\n\t%s" % (src, tgt, str(why))
                result = -1
                time.sleep(tries)
            tries += 1

        if result == 0:
            try:
                os.chmod(tgt, 493) # 0755
            except:
                pass
        elif self.optional:
            result = 0

        return result

    def runnable_status(self):
        if super(copy_outputs, self).runnable_status() == -1:
            return -1

        # if we have no signature on our output node, it means the previous build was terminated or died
        # before we had a chance to write out our cache, we need to recompute it by redoing this task once.
        if not getattr(self.outputs[0], 'sig', None):
            return RUN_ME

        src = self.inputs[0].abspath()
        tgt = self.outputs[0].abspath()

        # If there any target file is missing, we have to copy
        try:
            stat_tgt = os.stat(tgt)
        except OSError:
            return RUN_ME

        # Now compare both file stats
        try:
            stat_src = os.stat(src)
        except OSError:
            pass
        else:
            if self.check_timestamp_and_size:
                # same size and identical timestamps -> make no copy
                if stat_src.st_mtime >= stat_tgt.st_mtime + 2 or stat_src.st_size != stat_tgt.st_size:
                    return RUN_ME
            else:
                return super(copy_outputs, self).runnable_status()

        # Everything fine, we can skip this task
        return SKIP_ME


class macapp(copy_outputs):
    color = 'PINK'


@taskgen_method
def copy_files(self, source_file, check_timestamp_and_size=True):

    # import libraries are only allowed to be copied if the copy import flag is set and we're a secondary
    # copy (the primary copy goes to the appropriate Bin64 directory)
    _, extension = os.path.splitext(source_file.abspath())
    is_import_library = self._type == 'shlib' and extension == '.lib'
    is_secondary_copy_install = getattr(self, 'is_secondary_copy_install', False)

    # figure out what copies should be done
    skip_primary_copy = is_import_library
    skip_secondary_copy = is_import_library and not is_secondary_copy_install

    def _create_sub_folder_copy_task(output_sub_folder_copy):
        if output_sub_folder_copy is not None and output_sub_folder_copy != output_sub_folder:
            output_node_copy = node.make_node(output_sub_folder_copy)
            output_node_copy = output_node_copy.make_node( os.path.basename(source_file.abspath()) )
            self.create_copy_task(source_file, output_node_copy, False, check_timestamp_and_size)

    # If there is an attribute for 'output_folder', then this is an override of the output target folder
    # Note that subfolder if present will still be applied
    output_nodes = self.bld.get_output_folders(self.bld.env['PLATFORM'], self.bld.env['CONFIGURATION'], self)

    for node in output_nodes:
        output_node = node
        output_sub_folder = getattr(self, 'output_sub_folder', None)
        if not skip_primary_copy:
            if output_sub_folder is not None:
                output_node = output_node.make_node(output_sub_folder)
            output_node = output_node.make_node( os.path.basename(source_file.abspath()) )
            self.create_copy_task(source_file, output_node, False, check_timestamp_and_size)

        # Special case to handle additional copies
        if not skip_secondary_copy:
            output_sub_folder_copy_attr = getattr(self, 'output_sub_folder_copy', None)
            if isinstance(output_sub_folder_copy_attr, str):
                _create_sub_folder_copy_task(output_sub_folder_copy_attr)
            elif isinstance(output_sub_folder_copy_attr, list):
                for output_sub_folder_copy_attr_item in output_sub_folder_copy_attr:
                    if isinstance(output_sub_folder_copy_attr_item, str):
                        _create_sub_folder_copy_task(output_sub_folder_copy_attr_item)
                    else:
                        Logs.warn("[WARN] attribute items in 'output_sub_folder_copy' must be a string.")


@taskgen_method
def create_copy_task(self, source_file, output_node, optional=False, check_timestamp_and_size=True):
    if not getattr(self.bld, 'existing_copy_tasks', None):
        self.bld.existing_copy_tasks = dict()

    if output_node in self.bld.existing_copy_tasks:
        Logs.debug('create_copy_task: skipping duplicate output node: (%s, %s)' % (output_node.abspath(), self))
    else:
        new_task = self.create_task('copy_outputs', source_file, output_node)
        new_task.optional = optional
        new_task.check_timestamp_and_size = check_timestamp_and_size
        self.bld.existing_copy_tasks[output_node] = new_task

    return self.bld.existing_copy_tasks[output_node]
	
@taskgen_method
def create_macapp_task(self, source_file, output_node, check_timestamp_and_size=True):
    new_task = self.create_task('macapp', source_file, output_node)
    new_task.check_timestamp_and_size = check_timestamp_and_size
    return new_task


if os.name == "nt":
    os_symlink = getattr(os, "symlink", None)
    if callable(os_symlink):
        pass
    else:
        def symlink_ms(source, link_name):
            import ctypes
            csl = ctypes.windll.kernel32.CreateSymbolicLinkW
            csl.argtypes = (ctypes.c_wchar_p, ctypes.c_wchar_p, ctypes.c_uint32)
            csl.restype = ctypes.c_ubyte
            flags = 1 if os.path.isdir(source) else 0
            try:
                if csl(link_name, source.replace('/', '\\'), flags) == 0:
                    raise ctypes.WinError()
            except:
                pass
        os.symlink = symlink_ms

class symlink_outputs(Task):
    """
    Class to handle symlinking
    """
    color = 'YELLOW'
    optional = False
    """If True, build doesn't fail if symlink fails."""

    def run(self):
        src = self.inputs[0].abspath()
        tgt = self.outputs[0].abspath()

        # Create output folder
        tries = 0
        dir = os.path.dirname(tgt)
        created = False
        while not created and tries < 10:
            try:
                os.makedirs(dir)
                created = True
            except OSError as ex:
                self.err_msg = "%s\nCould not mkdir for symlink %s -> %s" % (str(ex), src, tgt)
                # ignore Directory already exists when this is called from multiple threads simultaneously
                if os.path.isdir(dir):
                    created = True
            tries += 1

        if not created:
            return -1

        tries = 0
        result = -1
        while result == -1 and tries < 10:
            try:
                os.symlink(src, tgt)
                result = 0
                self.err_msg = None
            except Exception as why:
                self.err_msg = "Could not perform symlink %s -> %s\n\t%s" % (src, tgt, str(why))
                result = -1
                time.sleep(tries)
            tries += 1

        if result != 0 and self.optional:
            result = 0

        return result

@taskgen_method
def create_symlink_task(self, source_file, output_node, optional=False, check_timestamp_and_size=True):
    if not getattr(self.bld, 'existing_symlink_tasks', None):
        self.bld.existing_symlink_tasks = dict()

    if output_node in self.bld.existing_symlink_tasks:
        Logs.debug('create_symlink_task: skipping duplicate output node: (%s, %s)' % (output_node.abspath(), self))
    else:
        new_task = self.create_task('symlink_outputs', source_file, output_node)
        new_task.optional = optional
        new_task.check_timestamp_and_size = check_timestamp_and_size #unused, keep only for compat with copy_task
        self.bld.existing_symlink_tasks[output_node] = new_task

    return self.bld.existing_symlink_tasks[output_node]


if os.name == "nt":
    os_hardlink = getattr(os, "link", None)
    if callable(os_hardlink):
        pass
    else:
        def hardlink_ms(source, link_name):
            import ctypes
            csl = ctypes.windll.kernel32.CreateHardLinkW
            csl.argtypes = (ctypes.c_wchar_p, ctypes.c_wchar_p, ctypes.c_uint32)
            csl.restype = ctypes.c_ubyte
            flags = 1 if os.path.isdir(source) else 0
            try:
                if csl(link_name, source.replace('/', '\\'), flags) == 0:
                    raise ctypes.WinError()
            except:
                pass
        os.link = hardlink_ms

class hardlink_outputs(Task):
    """
    Class to handle hardlinking
    """
    color = 'YELLOW'
    optional = False
    """If True, build doesn't fail if hardlink fails."""

    def run(self):
        src = self.inputs[0].abspath()
        tgt = self.outputs[0].abspath()

        # Create output folder
        tries = 0
        dir = os.path.dirname(tgt)
        created = False
        while not created and tries < 10:
            try:
                os.makedirs(dir)
                created = True
            except OSError as ex:
                self.err_msg = "%s\nCould not mkdir for hardlink %s -> %s" % (str(ex), src, tgt)
                # ignore Directory already exists when this is called from multiple threads simultaneously
                if os.path.isdir(dir):
                    created = True
            tries += 1

        if not created:
            return -1

        tries = 0
        result = -1
        while result == -1 and tries < 10:
            try:
                os.link(src, tgt)
                result = 0
                self.err_msg = None
            except Exception as why:
                self.err_msg = "Could not perform hardlink %s -> %s\n\t%s" % (src, tgt, str(why))
                result = -1
                time.sleep(tries)
            tries += 1

        if result != 0 and self.optional:
            result = 0

        return result

@taskgen_method
def create_hardlink_task(self, source_file, output_node, optional=False, check_timestamp_and_size=True):
    if not getattr(self.bld, 'existing_hardlink_tasks', None):
        self.bld.existing_hardlink_tasks = dict()

    if output_node in self.bld.existing_hardlink_tasks:
        Logs.debug('create_hardlink_task: skipping duplicate output node: (%s, %s)' % (output_node.abspath(), self))
    else:
        new_task = self.create_task('hardlink_outputs', source_file, output_node)
        new_task.optional = optional
        new_task.check_timestamp_and_size = check_timestamp_and_size #unused, keep only for compat with copy_task
        self.bld.existing_hardlink_tasks[output_node] = new_task

    return self.bld.existing_hardlink_tasks[output_node]



@taskgen_method
def create_hardlink_if_same_device_else_copy_task(self, source_file, output_node, optional=False, check_timestamp_and_size=True):
    if not getattr(self.bld, 'existing_hardlink_tasks', None):
        self.bld.existing_hardlink_tasks = dict()
    if not getattr(self.bld, 'existing_symlink_tasks', None):
        self.bld.existing_symlink_tasks = dict()
    if not getattr(self.bld, 'existing_copy_tasks', None):
        self.bld.existing_copy_tasks = dict()

    if output_node in self.bld.existing_hardlink_tasks or output_node in self.bld.existing_symlink_tasks or output_node in self.bld.existing_copy_tasks:
        Logs.debug('create_hardlink_if_same_device_else_copy_task: skipping duplicate output node: (%s, %s)' % (output_node.abspath(), self))
        return None
    else:
        (src_drive, src_tail) = os.path.splitdrive(source_file.abspath())
        (output_drive, output_tail) = os.path.splitdrive(output_node.abspath())
        if src_drive.lower() == output_drive.lower():
            return create_hardlink_task(self, source_file, output_node, optional, check_timestamp_and_size)
        else:
            return create_copy_task(self, source_file, output_node, optional, check_timestamp_and_size)


###############################################################################
@taskgen_method
def copy_dependent_objects(self, source_file, source_node, target_node, source_exclusion_list, build_folder_tree_only=False, flatten_target=False):
    """
    Copy dependent objects to a target node with pattern rules
    :param self:                    Context
    :param source_file:             Source filename
    :param source_node:             Source Node
    :param target_node:             Target (Destination) Node
    :param source_exclusion_list:   List of files to exclude
    :param build_folder_tree_only:  Only build folders as necessary (no file copy)
    :param flatten_target:          Flatten the output if the source is a tree
    :return:
    """
    flatten_subfolder = ''
    if isinstance(source_file, str):
        source_file_name = source_file
    elif isinstance(source_file, tuple):
        source_file_name = source_file[0]
        flatten_subfolder = source_file[1]+'/'

    if not source_node.abspath() == target_node.abspath():

        source_file_node = source_node.make_node(source_file_name)
        if flatten_target:
            source_path_path, source_path_filename = os.path.split(source_file_name)
            target_file_node = target_node.make_node(flatten_subfolder + source_path_filename)
        else:
            target_file_node = target_node.make_node(source_file_name)
        target_node_root_path = target_node.abspath()
        source_file_path = source_file_node.abspath()

        # Check if this is a file pattern, if so, glob the list of files
        if '*' in source_file_path:

            # Collect the results of the glob
            glob_items = glob.glob(source_file_path)
            for glob_item in glob_items:

                # For each result of the glob item, determine the subpath, which is the path of the source minus the root source node
                # e.g. if the source node root is c:/source and the glob result file is c:/source/path_a/file_b.xml, then the subpath would be
                # path_a
                glob_sub_path = os.path.dirname(glob_item[len(source_node.abspath())+1:])

                if not flatten_target:
                    # Calculate the glob target directory by taking the glob result (source) and replacing the source root node with the target root
                    # node's path.  e.g. c:/source/path_a -> c:/target/path_a and then create the folder if its missing.
                    glob_target_dir = os.path.dirname(os.path.join(target_node_root_path,glob_item[:len(source_file_path)]))
                    if not os.path.exists( glob_target_dir ):
                        os.makedirs( glob_target_dir )

                # Get the raw GLOB'ed filename
                glob_item_file_name = os.path.basename(glob_item)

                # Construct the source and target node based on the actual result glob'd file and recursively call this method again
                glob_source_node = source_node.make_node(glob_sub_path)
                if not flatten_target:
                    glob_target_node = target_node.make_node(glob_sub_path)
                else:
                    glob_target_node = target_node

                self.copy_dependent_objects(glob_item_file_name,glob_source_node,glob_target_node,source_exclusion_list,build_folder_tree_only,flatten_target)

        # Check if this is a file, perform the file copy task if needed
        elif os.path.isfile(source_file_path):
            # Make sure that the base folder for the file exists
            file_item_target_path = os.path.dirname(target_file_node.abspath())
            if not os.path.exists( file_item_target_path ):
                os.makedirs( file_item_target_path )

            if not build_folder_tree_only:
                self.create_copy_task(source_file_node, target_file_node)
        # Check if this is a folder, make sure the path exists and recursively act on the folder
        elif os.path.isdir(source_file_path):
            folder_items = os.listdir(source_file_path)

            # Make sure the target path will exist so we can recursively copy into it
            target_sub_node = target_file_node
            target_sub_node_path = target_sub_node.abspath()

            if not os.path.exists( target_sub_node_path ):
                os.makedirs( target_sub_node_path )

            for sub_item in folder_items:
                self.copy_dependent_objects(sub_item, source_file_node, target_sub_node, source_exclusion_list, build_folder_tree_only, flatten_target)


@after_method('set_pdb_flags')
@feature('c', 'cxx')
def add_copy_artifacts(self):
    """
    Function to generate the copy tasks to the target Bin64(.XXX) folder.  This will take any collection of source artifacts and copy them flattened into the Bin64(.XXX) target folder
    :param self: Context
    """
    if self.bld.env['PLATFORM'] == 'project_generator':
        return

    include_source_artifacts =  getattr(self, 'source_artifacts_include', [])
    if len(include_source_artifacts) == 0:
        return

    exclude_source_artifacts =  getattr(self, 'source_artifacts_exclude', [])
    current_platform = self.bld.env['PLATFORM']
    current_configuration = self.bld.env['CONFIGURATION']

    source_node = self.bld.path.make_node('')

    output_sub_folder = getattr(self, 'output_sub_folder', None)

    # If we have a custom output folder, then make a list of nodes from it
    target_folders = self.bld.get_output_folders(current_platform, current_configuration, self)

    # Copy to each output folder target node
    for target_node in target_folders:

        if output_sub_folder:
            target_node = target_node.make_node(output_sub_folder)

        # Skip if the source and target folders are the same
        if source_node.abspath() == target_node.abspath():
            continue

        for dependent_files in include_source_artifacts:
            self.copy_dependent_objects(dependent_files,source_node,target_node,exclude_source_artifacts,False,True)


@after_method('set_pdb_flags')
@feature('c', 'cxx')
def add_mirror_artifacts(self):
    """
    Function to generate the copy tasks for mirroring artifacts from Bin64.  This will take files that
    are relative to the base Bin64 folder and mirror them (copying them along with their folder structure)
    to any target Target folder (such as Bin64.Debug)
    :param self:    Context
    """
    if self.bld.env['PLATFORM'] == 'project_generator':
        return

    artifact_include_files = getattr(self, 'mirror_artifacts_to_include', [])

    # Ignore if there are no mirror artifacts to include
    if len(artifact_include_files) == 0:
        return

    artifact_exclude_files = getattr(self, 'mirror_artifacts_to_exclude', [])
    current_platform       = self.bld.env['PLATFORM']
    current_configuration  = self.bld.env['CONFIGURATION']

    # source node is Bin64 for win_x64 platform and BinMac64 for mac platform
    if self.bld.is_windows_platform(current_platform):
        source_node = self.bld.path.make_node('Bin64')
    elif self.bld.is_mac_platform(current_platform):
        source_node = self.bld.path.make_node('BinMac64')
    else:
        source_node = self.bld.path.make_node('Bin64') #default

    output_folders = self.bld.get_output_folders(current_platform, current_configuration, self)
    for target_node in output_folders:

        # Skip if the output folder is Bin64 already (this is where the source is)
        if target_node.abspath() == source_node.abspath():
            continue

        # Build the file exclusion list based off of the source node
        exclusion_abs_path_list = set()
        for exclude_file in artifact_exclude_files:
            normalized_exclude = os.path.normpath(os.path.join(source_node.abspath(),exclude_file))
            exclusion_abs_path_list.add(normalized_exclude)

        # Copy each file/folder in the collection
        # (first pass create the tree structure)
        for dependent_files in artifact_include_files:
            self.copy_dependent_objects(dependent_files,source_node,target_node,exclusion_abs_path_list,True)
        # (second pass create the tree structure)
        for dependent_files in artifact_include_files:
            self.copy_dependent_objects(dependent_files,source_node,target_node,exclusion_abs_path_list,False)


@after_method('set_pdb_flags')
@feature('c', 'cxx', 'copy_3rd_party_binaries')
def add_copy_3rd_party_artifacts(self):
    if self.bld.env['PLATFORM'] == 'project_generator':
        return

    third_party_artifacts = self.env['COPY_3RD_PARTY_ARTIFACTS']
    current_platform = self.bld.env['PLATFORM']
    current_configuration = self.bld.env['CONFIGURATION']

    if third_party_artifacts:

        copied_files = 0
        # Iterate through all target output folders
        for target_node in self.bld.get_output_folders(current_platform, current_configuration, self):

            # Determine the final output directory
            output_sub_folder = getattr(self, 'output_sub_folder', None)
            if output_sub_folder:
                # If the output subfolder is blacklisted, do not copy the dependency
                if output_sub_folder in DEPENDENT_TARGET_BLACKLISTED_SUBFOLDERS:
                    return
                output_path_node = target_node.make_node(output_sub_folder)
            else:
                output_path_node = target_node

            target_folder = output_path_node.abspath()

            for source_node in third_party_artifacts:
                source_full_path = source_node.abspath()
                source_filename = os.path.basename(source_full_path)
                target_full_path = os.path.join(target_folder, source_filename)
                if should_overwrite_file(source_full_path, target_full_path):
                    try:
                        # In case the file is readonly, we'll remove the existing file first
                        if os.path.exists(target_full_path):
                            os.chmod(target_full_path, stat.S_IWRITE)
                        fast_copy2(source_full_path, target_full_path)
                        copied_files += 1
                    except:
                        Logs.warn('[WARN] Unable to copy {} to destination {}.  '
                                  'Check the file permissions or any process that may be locking it.'
                                  .format(source_full_path, target_full_path))
        if copied_files > 0 and Logs.verbose > 0:
            Logs.info('[INFO] {} External files copied.'.format(copied_files))


@feature('copy_external_files')
@before_method('process_source')
def copy_external_files(self):
    """
    Feature to process copying external (files outside of the WAF root) folder as part of the build
    """

    if self.bld.env['PLATFORM'] == 'project_generator':
        return

    if 'COPY_EXTERNAL_FILES' not in self.env:
        return

    external_files = self.env['COPY_EXTERNAL_FILES']
    current_platform = self.bld.env['PLATFORM']
    current_configuration = self.bld.env['CONFIGURATION']

    copied_files = 0

    def _copy_single_file(src_file, tgt_file):
        if should_overwrite_file(src_file, tgt_file):
            try:
                # In case the file is readonly, we'll remove the existing file first
                if os.path.exists(tgt_file):
                    os.chmod(target_file, stat.S_IWRITE)
                fast_copy2(copy_external_file, target_file)
            except:
                Logs.warn('[WARN] Unable to copy {} to destination {}.  '
                          'Check the file permissions or any process that may be locking it.'
                          .format(copy_external_file, target_file))

    # Iterate through all target output folders
    for target_node in self.bld.get_output_folders(current_platform, current_configuration, self):

        if hasattr(self, 'output_sub_folder'):
            output_path = os.path.join(target_node.abspath(), self.output_sub_folder)
        else:
            output_path = target_node.abspath()

        for copy_external_file in external_files:
            if not os.path.exists(copy_external_file):
                continue
            filename = os.path.split(copy_external_file)[1]
            target_file = os.path.join(output_path, filename)

            if should_overwrite_file(copy_external_file, target_file):
                try:
                    # In case the file is readonly, we'll remove the existing file first
                    if os.path.exists(target_file):
                        os.chmod(target_file, stat.S_IWRITE)
                    fast_copy2(copy_external_file, target_file)
                except:
                    Logs.warn('[WARN] Unable to copy {} to destination {}.  '
                              'Check the file permissions or any process that may be locking it.'
                                .format(copy_external_file, target_file))
                copied_files += 1
    if copied_files > 0:
        Logs.info('[INFO] {} External files copied.'.format(copied_files))


@feature('c', 'cxx', 'copy_3rd_party_binaries')
@before_method('process_source')
def copy_3rd_party_binaries(self):
    """
    Feature that will copy any library that is specified as a uselib to the target output folder
    :param self:    Context
    """

    # Skip non-build commands
    if self.bld.cmd in ('msvs', 'android_studio'):
        return

    def _process_filelist(source_files):

        if 'COPY_3RD_PARTY_ARTIFACTS' not in self.env:
            self.env['COPY_3RD_PARTY_ARTIFACTS'] = []

        for source_file in source_files:

            source_file_file_norm_path = os.path.normpath(source_file)

            source_node = self.bld.root.make_node(source_file_file_norm_path)
            self.env['COPY_3RD_PARTY_ARTIFACTS'] += [source_node]

    uselib_keys = []
    uselib_keys += getattr(self, 'uselib', [])
    uselib_keys += getattr(self, 'use', [])
    if len(uselib_keys)>0:
        for uselib_key in uselib_keys:

            def _extract_full_pathnames(path_varlib_key, filename_varlib_key):
                uselib_path_varname = '{}_{}'.format(path_varlib_key, uselib_key)
                uselib_filename_varname = '{}_{}'.format(filename_varlib_key, uselib_key)
                fullpaths = []
                if uselib_path_varname in self.env and uselib_filename_varname in self.env:
                    source_paths = self.env[uselib_path_varname]
                    source_files = self.env[uselib_filename_varname]
                    # Keep track and warn for duplicates that will be ignored
                    process_source_filename = set()
                    for source_path in source_paths:
                        for source_file in source_files:
                            sharedlib_fullpath = os.path.normpath(os.path.join(source_path, source_file))
                            if os.path.exists(sharedlib_fullpath):
                                if source_file in process_source_filename:
                                    Logs.warn('[WARN] Duplicate dependent shared file detected ({}).  The second copy will be ignored'.format(source_file))
                                else:
                                    fullpaths.append(sharedlib_fullpath)
                                    process_source_filename.add(source_file)
                return fullpaths

            # Process the shared lib files if any
            shared_fullpaths = _extract_full_pathnames('SHAREDLIBPATH', 'SHAREDLIB')
            if len(shared_fullpaths) > 0:
                _process_filelist(shared_fullpaths)

            # Process the pdbs if any

            if self.bld.is_option_true('copy_3rd_party_pdbs'):
                shared_pdbs = _extract_full_pathnames('SHAREDLIBPATH', 'PDB')
                if len(shared_pdbs) > 0:
                    _process_filelist(shared_pdbs)


@feature('c', 'cxx', 'copy_3rd_party_extras')
@before_method('process_source')
def copy_3rd_party_extras(self):
    """
    Feature that will copy 'copy_extra' values that are defined by the 3rd party framework to the target bin folder
    """

    # Skip non-build commands
    if self.bld.cmd in ('msvs', 'android_studio'):
        return

    current_platform = self.bld.env['PLATFORM']
    current_configuration = self.bld.env['CONFIGURATION']

    copy_extra_commands = set()
    visited_uselib_keys = set()

    def _collect_copy_commands(copy_extra_key):

        # Look up the copy_extra command value
        if copy_extra_key not in self.env:
            return

        copy_extra_raw_value = self.env[copy_extra_key]
        if isinstance(copy_extra_raw_value,str) and copy_extra_raw_value.startswith('@'):

            # This value is an alias for an actual value, lookup the actual value
            copy_extra_alias_name = 'COPY_EXTRA_{}'.format(copy_extra_raw_value[1:])
            if copy_extra_alias_name not in self.env:
                Logs.warn("[WARN] copy_extra alias '{}' refers to an invalid entry ({}) ".format(copy_extra_raw_value, copy_extra_alias_name))
                return

            # The aliased value must be a concrete value (not another alias)
            copy_extra_aliased_raw_value = self.env[copy_extra_alias_name]
            if not isinstance(copy_extra_aliased_raw_value, list):
                Logs.warn("[WARN] copy_extra alias '{}' refers to another aliased value ({}) ".format(copy_extra_raw_value, copy_extra_alias_name))
                return

            uselib_copy_commands = copy_extra_aliased_raw_value
            visited_uselib_keys.add(copy_extra_alias_name)
        else:
            uselib_copy_commands = copy_extra_raw_value

        if uselib_copy_commands is not None:
            for uselib_copy_command in uselib_copy_commands:
                copy_extra_commands.add(uselib_copy_command)

    def _process_copy_command(input_copy_extra_command):

        # Special case: If this is a windows host platform, then it may contain the drive letter
        # followed by a ':'.  This token is used for the commands, so we need to take this into account
        copy_extra_command_parts = input_copy_extra_command.split(':')
        if Utils.unversioned_sys_platform() == 'win32' and len(copy_extra_command_parts) == 3:
            source = '{}:{}'.format(copy_extra_command_parts[0], copy_extra_command_parts[1])
            destination = copy_extra_command_parts[2]
        elif len(copy_extra_command_parts) == 2:
            source = copy_extra_command_parts[0]
            destination = copy_extra_command_parts[1]
        else:
            Logs.warn("[WARN] Copy Extra rule is invalid ({})", copy_extra_command)
            return False

        skip_pdbs = not self.bld.is_option_true('copy_3rd_party_pdbs')

        if os.path.isdir(source):
            glob_syntax = '{}/**/*'.format(source)
            glob_results = glob.glob(glob_syntax)
            raw_src_and_tgt = []
            for glob_result in glob_results:
                source_file = os.path.normpath(glob_result)
                if os.path.isdir(source_file):
                    continue
                source_file_relative = source_file[len(source):]

                if skip_pdbs and source_file_relative.upper().endswith('.PDB'):
                    continue

                dest_file = os.path.join(destination,os.path.basename(source)) + source_file_relative
                raw_src_and_tgt.append( (source_file,dest_file) )
        elif os.path.exists(source):
            source_file = source
            target_file = os.path.join(destination, os.path.basename(source))
            raw_src_and_tgt = [(source_file,target_file)]
        else:
            Logs.warn("Cannot copy source '{}' File not found.  Skipping.".format(source))
            return False

        for target_node in self.bld.get_output_folders(current_platform, current_configuration, self):
            for source_file, target_file in raw_src_and_tgt:
                source = self.bld.root.make_node(source_file)
                target = target_node.make_node(target_file)
                self.create_copy_task(source, target)

        return True


    uselib_keys = []
    uselib_keys += getattr(self, 'uselib', [])
    uselib_keys += getattr(self, 'use', [])
    if uselib_keys:
        for uselib_key in uselib_keys:

            copy_extra_key = 'COPY_EXTRA_{}'.format(uselib_key)
            # Skip previously visited entries
            if copy_extra_key in visited_uselib_keys:
                continue
            _collect_copy_commands(copy_extra_key)

    if len(copy_extra_commands)>0:
        for copy_extra_command in copy_extra_commands:
            _process_copy_command(copy_extra_command)


@feature('copy_module_dependent_files')
@before_method('process_source')
def copy_module_dependent_files(self):
    """
    Feature to process copying external module dependent files (files that are not directly
    part of any module build) to the target output folder
    """

    if self.bld.env['PLATFORM'] == 'project_generator':
        return

    copy_dependent_env_key = 'COPY_DEPENDENT_FILES_{}'.format(self.target.upper())
    if copy_dependent_env_key not in self.env:
        return

    external_files = self.env[copy_dependent_env_key]
    current_platform = self.bld.env['PLATFORM']
    current_configuration = self.bld.env['CONFIGURATION']

    copied_files = 0

    def _copy_single_file(src_file, tgt_folder):

        src_filename = os.path.split(src_file)[1]
        tgt_file = os.path.join(tgt_folder, src_filename)

        if should_overwrite_file(src_file, tgt_file):
            try:
                # In case the file is readonly, we'll remove the existing file first
                if os.path.exists(tgt_file):
                    os.chmod(tgt_file, stat.S_IWRITE)
                fast_copy2(src_file, tgt_file)
                return True
            except:
                Logs.warn('[WARN] Unable to copy {} to destination {}.  '
                          'Check the file permissions or any process that may be locking it.'
                          .format(copy_external_file, tgt_file))
            return False

    # Iterate through all target output folders
    for target_node in self.bld.get_output_folders(current_platform, current_configuration, self):

        output_paths = []
        if getattr(self, 'output_sub_folder', None):
            output_sub_folder_value = flatten_list(self.output_sub_folder)
            for output_sub_folder in output_sub_folder_value:
                output_paths.append(os.path.join(target_node.abspath(), output_sub_folder))
        else:
            output_paths.append(target_node.abspath())

        for output_path in output_paths:

            for copy_external_file in external_files:

                # Is this a potential wildcard pattern?
                is_pattern = '*' in os.path.basename(copy_external_file)
                if is_pattern:
                    # If this is a wildcard pattern, make sure its base directory is valid
                    if not os.path.exists(os.path.dirname(copy_external_file)):
                        continue
                    files_from_pattern = glob.glob(copy_external_file)
                    for file_from_pattern in files_from_pattern:
                        if _copy_single_file(file_from_pattern, output_path):
                            copied_files += 1
                else:
                    # Skip if the file does not exist or is a directory
                    if not os.path.exists(copy_external_file):
                        continue
                    if os.path.isdir(copy_external_file):
                        continue
                    if _copy_single_file(copy_external_file, output_path):
                        copied_files += 1

    if copied_files > 0:
        Logs.info('[INFO] {} dependent files copied for target {}.'.format(copied_files, self.target))
