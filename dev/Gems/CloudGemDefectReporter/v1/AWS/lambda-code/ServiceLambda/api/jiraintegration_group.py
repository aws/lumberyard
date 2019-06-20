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
import jira_integration
import additonal_report_Info
import CloudCanvas
import json
import re
import copy

#
#   Group defect submissions from the Cloud Gem Portal Cloud Gem Defect Reporter.
#   The group mapping and the defect report event information is passed as arguments.
#   The group mapping needs to be applied to each defect report event.
#
@service.api
def post(request, group):    
    jira_integration_settings = group['groupContent']
    reports = group['reports']
    
    prepared_reports = jira_integration.prepare_jira_tickets(reports, jira_integration_settings)    
    jira_integration.create_Jira_tickets(prepared_reports)     

    return {'status': 'SUCCESS'}
