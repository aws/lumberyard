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
import pytest


from copy_tasks import preprocess_pathlen_for_windows
@pytest.mark.parametrize(
        "input, expected", [
            ('c:\\blah\\blah\\blah\\blah\\blah\\blah\\blah', 'c:\\blah\\blah\\blah\\blah\\blah\\blah\\blah'),  # short enough expect same path
            (u'\\\\?\\c:\\blah\\blah\\blah\\blah\\blah\\blah\\blah', u'\\\\?\\c:\\blah\\blah\\blah\\blah\\blah\\blah\\blah'),  # unc already, expect same unc path
            ('c:\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah', u'\\\\?\\c:\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah\\blah'),  # too long expect a unc path
        ])

def test_preprocess_pathlen_for_windows(input, expected):
    actual = preprocess_pathlen_for_windows(input)
    assert expected == actual

