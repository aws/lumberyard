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
# $Revision: #4 $

from distutils.version import LooseVersion

# Our version numbers should conform to http://semver.org/ 2.0.
class Version(LooseVersion):

    def __init__(self, s):
        # LooseVersion is an old style class, so can't use super
        LooseVersion.__init__(self, s) 

    @property
    def major(self):
        return self.version[0]

    @property
    def minor(self):
        return self.version[1]

    @property
    def revision(self):
        return self.version[2]

    def is_compatible_with(self, other):
        
        if not isinstance(other, Version):
            other = Version(other)

        if self.major != other.major:
            return False
        
        # self.major == other.major

        if self.minor < other.minor:
            return False

        if self.minor > other.minor:
            return True

        # self.minor == other.minor

        if self.revision < other.revision:
            return False

        # self.revision == self.revions

        return True


