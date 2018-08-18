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
import dynamic_content_constant as c
import portal_request as pr


#
# Cloud Gem Dynamic Content Load Test Teardown API
#
@service.api
def post(request, test_data):
    print("{} load test clean up entered.".format(c.GEM_NAME))

    pr.set_file_statuses(request, test_data)

    print("{} load test clean up has finished.".format(c.GEM_NAME))
    return "success"
