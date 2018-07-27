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

import service
import os
import metric_constant as c
import errors
from aws_lambda import Lambda
from cgf_utils import custom_resource_response

@service.api
def consume(request):
    context=dict({})    
    lb = Lambda(context)
    response = lb.invoke(os.environ[c.ENV_LAMBDA_CONSUMER_LAUNCHER])    
    return custom_resource_response.success_response({}, "*")