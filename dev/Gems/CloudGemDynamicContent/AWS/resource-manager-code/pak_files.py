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

from __future__ import print_function

import errno
import hashlib
import os
import os.path
import posixpath
import shutil
import stat
import subprocess
import sys
import tempfile
import zipfile

from path_utils import ensure_posix_path

# For Debugging, flip this on to get verbose output
vprint = lambda *a: None


# vprint = lambda *args: print("\n".join(args))


class PakFileArchiver:

    def _run_command(self, arguments):
        arguments = [self.archive_tool] + arguments
        vprint("Trying command: {}".format(" ".join(arguments)))
        try:
            subprocess.check_call(arguments)
        except subprocess.CalledProcessError as e:
            print("Archive command error: ", e.output)

    def update_pak(self, file_to_add_path, dest_pak_path):
        """ Adds a file to an existing pak file """
        file_dir_name = os.path.dirname(file_to_add_path)

        self.write_pak(file_to_add_path, file_dir_name, dest_pak_path, 'a')

    def archive_folder(self, src_folder_path, dest_pak_path):
        """ Creates a pak file of all the files in the provided source folder """
        dest_pak_dir = os.path.dirname(dest_pak_path)
        if not os.path.exists(dest_pak_dir):
            os.makedirs(dest_pak_dir)

        self.write_pak(src_folder_path, src_folder_path, dest_pak_path, 'w')

    def write_pak(self, src_path, src_dir_name, dest_pak_path, write_option):
        with zipfile.ZipFile(dest_pak_path, write_option) as myzip:
            for dir_path, subfolder_names, file_names in os.walk(src_path):
                for subfolder_name in subfolder_names:
                    subfolder_path = os.path.join(dir_path, subfolder_name)
                    arcname = subfolder_path.replace(src_dir_name + os.path.sep, '')
                    myzip.write(subfolder_path, arcname)

                for file_name in file_names:
                    file_path = os.path.join(dir_path, file_name)
                    arcname = file_path.replace(src_dir_name + os.path.sep, '')
                    myzip.write(file_path, arcname)

    def archive_files(self, src_file_list, dest_pak_path):
        """ create a pak file with directory preserved

        Arguments
        src_file_list -- list of pairs of absolute paths to files to pack and the portion
                            of the path that represents what to use as the root of the file
                            so that the pack has the correct relative folder structure
        dest_pak_path -- absolute path and file name of the pak file to create
        """

        # create a temporary folder
        temp_dir = tempfile.mkdtemp()
        vprint("Create temp dir: " + temp_dir)

        # copy the files to the temporary folder
        for file, src_file_root in src_file_list:
            file = os.path.normpath(file)
            file = os.path.normcase(file)
            vprint("\n\tProcessing " + file)
            # verify the file starts with the root
            src_file_root = os.path.normpath(src_file_root)
            src_file_root = os.path.normcase(src_file_root)
            vprint("\tSource file root is " + src_file_root)
            if not file.startswith(src_file_root):
                print("File doesn't have correct root. Root is: {} and file path is: {}".format(src_file_root, file))
            else:
                relative_path = ensure_posix_path(file[len(src_file_root):])

                # Be sure we aren't starting with a file separator or the join below will fail
                if len(relative_path) and relative_path[0] == posixpath.sep:
                    relative_path = relative_path[1:]
                vprint("\tRelative path " + relative_path)
                temp_path = os.path.join(temp_dir, relative_path)
                vprint("\tTemp path " + temp_path)
                if not os.path.exists(os.path.dirname(temp_path)):
                    try:
                        os.makedirs(os.path.dirname(temp_path))
                    except OSError as exc:  # guard against race condition
                        if exc.errno != errno.EEXIST:
                            raise
                shutil.copy(file, temp_path)
                # clear read only flags in case this came from source control environment so the temp folder
                # can be removed when done
                os.chmod(temp_path, stat.S_IWUSR | stat.S_IRUSR | stat.S_IWGRP | stat.S_IRGRP |
                         stat.S_IWOTH | stat.S_IROTH)

        # create a pak archive from the files in the temporary folder
        self.archive_folder(temp_dir, dest_pak_path)

        # delete the temporary files and folder
        shutil.rmtree(temp_dir)


def extract_from_pak_object(pak_object, file_path, output):
    with zipfile.ZipFile(pak_object) as manifest_pak:
        file_to_extract = manifest_pak.open(file_path)
        with open(output, 'wb') as outfile:
            shutil.copyfileobj(file_to_extract, outfile)


def calculate_pak_content_hash(pak_path: str) -> str:
    """ Calculate hash of all of the individual file hash inside the pak
    
    Arguments
        pak_path -- path to the pak file
    """
    content_hash = hashlib.md5()
    with zipfile.ZipFile(pak_path, 'r') as myzip:
        # get a list of archive files in the pak
        archive_file_list = [member for member in myzip.namelist() if not member.endswith('/')]
        for archive_file in archive_file_list:
            # update the pak content hash using the hash of each archive file
            with myzip.open(archive_file) as myfile:
                file_hash = hashlib.md5(myfile.read()).hexdigest()
                content_hash.update(file_hash.encode())

    return content_hash.hexdigest()


def main():
    """This code is here to make it easy to archive without needing to run the lmbr_aws utility"""
    global vprint
    vprint = lambda *args: print("\n".join(args))

    if len(sys.argv) != 3:
        print("Error: two parameters expected. pak_files.py <create|update> <test data file path>")
        exit(-1)

    # for test purposes there are two parameters expected
    #   first parameter is the command, either 'create' or 'update'
    #   second parameter is the test data file which has the following structure
    #   depending on the command
    #
    #       command 'create'
    #       the first line being the path to the output pak file
    #       the second line being the root
    #       the remaining lines are absolute paths to files to archive
    #       example:
    #          c:/assets
    #          c:/assets/animations/skeleton.xml
    #          c:/assets/animations/chicken/chicken.caf
    #          c:/assets/levels/level.xml
    #          etc
    #
    #       command 'update'
    #       the first line being the absolute path to the pak file to update
    #       the second line being the absolute path to the file to add to the pak

    # assumes the script is being run from somewhere in a lumberyard project
    dev_root = os.getcwd()
    cur_begin, cur_end = os.path.split(dev_root)
    while cur_end != "dev" and cur_end != "":
        cur_begin, cur_end = os.path.split(cur_begin)
    if cur_end == "":
        print("Error: Can't find dev folder, you likely aren't in a Lumberyard game subfolder.")
        exit(-1)
    dev_root = os.path.join(cur_begin, "dev")
    vprint("Dev root is " + dev_root)

    # set up enough information in the config context that would normally come from lmbr_aws
    class Context:
        def __init__(self):
            class Config:
                def __init__(self):
                    self.root_directory_path = dev_root

            self.config = Config()

    context = Context()

    archiver = PakFileArchiver()

    test_data = [line.strip() for line in open(sys.argv[2], 'r')]

    test_dest_pak = test_data[0]
    test_src_root = test_data[1]

    command = sys.argv[1].lower()
    if command == 'create':
        test_src_list = [(file_path, test_src_root) for file_path in test_data[2:]]
        archiver.archive_files(test_src_list, test_dest_pak)
    elif command == 'update':
        archiver.update_pak(test_src_root, test_dest_pak)
    else:
        print("Unknown command " + command)
        exit(-1)


if __name__ == "__main__":
    main()
