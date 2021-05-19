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
# $Revision: #1 $
import platform
import posixpath


def ensure_posix_path(file_path):
    """
    On Windows convert to standard pathing for DynamicContent which is posix style '/' path separators.

    This is because Windows operations can handle both pathing styles
    """
    if file_path is not None and platform.system() == 'Windows':
        return file_path.replace('\\', posixpath.sep)
    return file_path
