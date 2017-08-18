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

import os, sys, codecs, fnmatch, stat

BUFSIZE = 4096
BOMLEN = len(codecs.BOM_UTF8)

NO_BOM_MESSAGE =        '  No BOM:         {}'
REMOVING_BOM_MESSAGE =  '  Removing BOM:   {}'
READ_ONLY_BOM_MESSAGE = '! ReadOnly BOM:   {}'
NON_EXISTANCE_MESSAGE = '! Does not exist: {}'

def file_has_bom(path):
    with open(path, "rb") as fp:
        chunk = fp.read(len(codecs.BOM_UTF8))
        return chunk.startswith(codecs.BOM_UTF8)

def remove_bom_from_file(path):
    with open(path, "r+b") as fp:
        chunk = fp.read(BUFSIZE)
        assert(chunk.startswith(codecs.BOM_UTF8))
        i = 0
        chunk = chunk[BOMLEN:]
        while chunk:
            fp.seek(i)
            fp.write(chunk)
            i += len(chunk)
            fp.seek(BOMLEN, os.SEEK_CUR)
            chunk = fp.read(BUFSIZE)
        fp.seek(-BOMLEN, os.SEEK_CUR)
        fp.truncate()

def remove_bom_from_directory(path, pattern):
    for child_name in os.listdir(path):
        child_path = os.path.join(path, child_name)
        if os.path.isdir(child_path):
            remove_bom_from_directory(child_path, pattern)
        elif fnmatch.fnmatch(child_name, pattern):
            if file_has_bom(child_path):
                if os.access(child_path, os.W_OK):
                    print REMOVING_BOM_MESSAGE.format(child_path)
                    remove_bom_from_file(child_path)
                else:
                    print READ_ONLY_BOM_MESSAGE.format(child_path)
            else:
                # print NO_BOM_MESSAGE.format(child_path)
                pass

if len(sys.argv) < 2:
    print 'Usage: remove_bom <PATTERN> [<DIR>...]'
    print ''
    print 'Will remove UTF-8 BOM from files whith names that match <PATTERN>'
    print 'found in the specified directories, recusivly. If no directories'
    print 'are specified, the current working direcotry is used.'
    exit(1)

pattern = sys.argv[1]
directories = sys.argv[2:] or [ os.getcwd() ]
for directory in directories:
    if os.path.isdir(directory):
        remove_bom_from_directory(directory, pattern)
    else:
        NON_EXISTANCE_MESSAGE.format(directory)

