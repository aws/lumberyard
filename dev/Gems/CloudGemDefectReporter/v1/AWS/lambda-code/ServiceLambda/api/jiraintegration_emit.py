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
import defect_reporter_s3 as s3_client
from botocore.client import Config

#
#   Emitted defect events from the Cloud Gem Metrics pipeline are received here.
#   If the Cloud Gem Defect Reporter is configured for automatic submission mode than the ticket is submitted.
#   If the Cloud Gem Defect Reporter is configured for manual mode the function exits.   
#
@service.api
def post(request, context):
    try:
        #tags on deployment may not enable JIRA.   By default JIRA is disabled.
        jira_integration_settings = jira_integration.get_jira_integration_settings()
    except:
        return {'status': 'FAILURE'}
    submit_mode = jira_integration_settings.get('submitMode', 'manual')
    
    if submit_mode != 'auto':
        return {'status': 'SUCCESS'}
    
    events_context = context['emitted']
    bucket = events_context['bucket']
    key = events_context['key']
    s3 = s3_client.get_client()
    res = s3.get_object(Bucket = bucket, Key = key)
    reports = json.loads(res['Body'].read())  
    prepared_reports = jira_integration.prepare_jira_tickets(reports, jira_integration_settings)
    
    return {'status': jira_integration.create_Jira_tickets(prepared_reports)}
    
