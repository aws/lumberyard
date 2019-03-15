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


def clean_stale_pycs(base_folder):
    """
    Perform simple stale pyc housekeeping (pyc files that dont have the source .py) since there are cases where
    :param base_folder: The folder to recurse into and clean up the stale pyc files
    """
    
    # Recurse through the package folder
    for root, _, files in os.walk(base_folder, topdown=True):
        for file in files:
            fullpath = os.path.join(root, file)
            extension = os.path.splitext(file)[1].lower()
            if extension == '.pyc':
                py_path = fullpath[:-1]
                if not os.path.isfile(py_path):
                    os.unlink(fullpath)


