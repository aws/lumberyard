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

class HandledError(Exception):
    '''Represents an error that can be displayed to the user without a stack trace.'''

    def __init__(self, msg, cause=None):
        self.msg = msg
        self.cause = cause

    def __str__(self):
        if self.cause is not None:
            return self.msg + ' ' + repr(self.cause)
        else:
            return self.msg
