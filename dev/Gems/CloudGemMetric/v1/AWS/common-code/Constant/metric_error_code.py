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

class Error(object):
    
    @staticmethod
    def exceeded_maximum_metric_capacity():
        return "ErrorExceededMaximumMetricCapacity"

    @staticmethod
    def missing_attribute():
        return "ErrorMissingAttributes"

    @staticmethod
    def is_not_lower():
        return "ErrorNotLowerCase"

    @staticmethod
    def out_of_order():
        return "ErrorOutOfOrder"

    @staticmethod
    def unable_to_sort():
        return "ErrorNotSorted"

    @staticmethod
    def is_null():
        return "ErrorNullValue"

    @staticmethod   
    def empty_dataframe():
        return "ErrorEmptyDataFrame"
    