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
import boto3
import CloudCanvas
import json

#
#   Manual single defect submissions from the Cloud Gem Portal Cloud Gem Defect Reporter.
#   Defect reports should have all mappings completed in the CGP and this is a direct submission to Jira at this point.
#   Reports passed here are already prepared.    
#
@service.api
def post(request, reports):    
    return {'status': jira_integration.create_Jira_tickets(reports)}