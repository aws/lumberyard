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
from copy import copy

import service
import dynamic_content_constant as c
import portal_request as pr


#
# Cloud Gem Dynamic Content Load Test Setup Initialization API
#
@service.api
def post(request):
    print("{} load test initialization entered.".format(c.GEM_NAME))

    out = {'FileList': []}

    set_files_to_public(request, out)

    print("{} load test initialization has finished.".format(c.GEM_NAME))
    return out


#
# Set all test files to Public if they aren't already
#
def set_files_to_public(request, out):
    paks = pr.list_all_content(request)

    for pak in paks['PortalFileInfoList']:
        if pak['StagingStatus'] is not c.STAGING_STATUS_PUBLIC:
            build_output(request, pak, out)
            modify_staging_status(request, pak)


#
# Build loadtest output data to pass to teardown
#
def build_output(request, pak, out):
    out['FileList'].append(pak)


#
# Modify the staging status of the target file
#
def modify_staging_status(request, pak):
    new = copy(pak)
    new['StagingStatus'] = c.STAGING_STATUS_PUBLIC

    file_list = {'FileList': []}
    file_list['FileList'].append(new)

    pr.set_file_statuses(request, file_list)
