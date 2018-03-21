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

import imp
import sys

def load_module(name, path, log_import=True):
    if log_import:
        print 'Loading module {} from {}.'.format(name, path)

    result = None
    imp.acquire_lock()

    try:
        fp, pathname, description = imp.find_module(name, [path])

        try:
            result = imp.load_module(name, fp, pathname, description)

        finally:
            if fp:
                fp.close()

    finally:
        imp.release_lock()

    return result
