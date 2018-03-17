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

from swagger_json_navigator import SwaggerNavigator

class SwaggerNavigatorMatcher(object):

    def __init__(self, expected_value):
        if isinstance(expected_value, SwaggerNavigator):
            expected_value = expected_value.value
        self.__expected_value = expected_value

    def __eq__(self, other):
        return isinstance(other, SwaggerNavigator) and other.value == self.__expected_value

    def __str__(self):
        return str(self.__expected_value)



