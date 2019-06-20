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


from utils import calculate_string_hash, is_value_true

import pytest

@pytest.mark.parametrize(
    "input, expected", [
        ('foo', 'acbd18db4cc2f85cedef654fccc4a4d8'),    # Expected MD5 hash of 'foo'
        ('bar', '37b51d194a7513e45b56f6524f2d51f2'),    # Expected MD5 hash of 'bar'
        ('', 'd41d8cd98f00b204e9800998ecf8427e'),       # Expected MD5 hash of ''
    ])
def test_CalculateStringHash_ValidInputValues_Success(input, expected):
    
    actual_hash = calculate_string_hash(input)
    assert expected == actual_hash


@pytest.mark.parametrize(
    "input_value, expected_result", [
        ('y', True),
        ('yes', True),
        ('t', True),
        ('1', True),
        ('Y', True),
        ('Yes', True),
        ('T', True),
        ('YES', True),
        ('n', False),
        ('no', False),
        ('f', False),
        ('0', False),
        ('N', False),
        ('No', False),
        ('F', False),
        ('NO', False),
        (True, True),
        (False, False)
    ])
def test_ConvertStringToBoolean_ValidInputs_Success(input_value, expected_result):
    actual_result = is_value_true(input_value)
    assert expected_result == actual_result


@pytest.mark.parametrize(
    "input_value", [
        ('Z'),
        ('z')
    ])
def test_ConvertStringToBoolean_InvalidInput_Exception(input_value):
    with pytest.raises(ValueError):
        is_value_true(input_value)

