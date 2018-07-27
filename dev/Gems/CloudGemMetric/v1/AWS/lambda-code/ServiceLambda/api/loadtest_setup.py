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

from __future__ import print_function

import service
import metric_constant as c

#  Import Admin APIs

#
# Cloud Gem Player Account Load Test Setup Initialization API
#
@service.api
def post(request):
    print("{} load test initialization entered.".format(c.RES_GEM_NAME))

    # Create output dictionary, interact with admin APIs, log changes that you want to revert in output dictionary

    #  No Setup Needed

    print("{} load test initialization has finished.".format(c.RES_GEM_NAME))
    return {}
