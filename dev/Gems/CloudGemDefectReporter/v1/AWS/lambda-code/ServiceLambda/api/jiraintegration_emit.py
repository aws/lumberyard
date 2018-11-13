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
import jiraintegration_group
import additonal_report_Info
import boto3
import CloudCanvas
import json
import re
import copy

@service.api
def post(request, context):
    try:
        #tags on deployment may not enable JIRA.   By default JIRA is disabled.
        jira_integration_settings = jira_integration.get_jira_integration_settings()
    except:
        return {'status': 'FAILURE'}
    submit_mode = jira_integration_settings.get('submitMode', 'manual')
    events_context = context['emitted']
    bucket = events_context['bucket']
    key = events_context['key']
    s3 = boto3.client('s3')
    
    res = s3.get_object(Bucket = bucket, Key = key)
    reports = json.loads(res['Body'].read())
    
    field_mappings = get_field_mappings(jira_integration_settings)
    
    mapped_reports = []
    for report in reports:
        mapped_report = copy.deepcopy(field_mappings)        
        jiraintegration_group.map_embedded_to_actual(mapped_report, report)
        mapped_report['attachment_id'] = json.loads(report['attachment_id'])
        mapped_report['universal_unique_identifier'] = report['universal_unique_identifier']
        mapped_reports.append(mapped_report)
    
    status = 'SUCCESS'
    if submit_mode == 'auto':
        status = jira_integration.create_Jira_tickets(mapped_reports)
    return {'status': status}

def get_field_mappings(jira_integration_settings):
    field_mappings = jira_integration.get_field_mappings(jira_integration_settings['project'], jira_integration_settings['issuetype'])
    result = {}
    for entry in field_mappings:
        if entry['mapping']:
            result[entry['id']] = entry['mapping']

    return result