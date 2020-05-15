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


def message(e):
    # type: (Exception) -> str
    """
    Gets the 'message' from an exception
    """
    # If we want the message, check if it exists
    # Supports Python2
    #
    # For python 3 call str will grab the message
    if hasattr(e, 'message'):
        return e.message
    else:
        return str(e)
