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
# $Revision$


MAX_LAMBDA_NAME_LENGTH = 64


def sanitize_lambda_name(lambda_name):
    result = lambda_name

    if len(lambda_name) > MAX_LAMBDA_NAME_LENGTH:
        digest = "-%x" % (hash(lambda_name) & 0xffffffff)
        result = lambda_name[:MAX_LAMBDA_NAME_LENGTH - len(digest)] + digest

    return result


def get_function_runtime(name, description, resources):
    function_runtime = description["Properties"]["Runtime"]
    # we are probably looking for a configuration
    if type(function_runtime) is type({}) and "Fn::GetAtt" in function_runtime:
        lambda_config = function_runtime["Fn::GetAtt"][0]
        function_runtime = resources[lambda_config]['Properties']["Runtime"]

    return function_runtime
