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

from cgf_utils import properties

class PropertiesMatcher(object):
    '''Matches properties._Properties objects.'''

    def __init__(self, src = None):
        self.__src = src

    def __eq__(self, other):

        if not isinstance(other, properties._Properties):
            return False

        if self.__src is not None:
            if self.__src != other.__dict__:
                return False

        return True
