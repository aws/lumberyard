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
import re
import copy

EMBEDDED_MAP_ENTRY_PATTERN = "\[\s*\w*\s*\]"

@service.api
def post(request, group):
    client = boto3.client('lambda')
    group_map = group['groupContent']
    reports = group['reports']
    
    mapped_reports = []
    for report in reports:
        mapped_report = copy.deepcopy(group_map)        
        map_embedded_to_actual(mapped_report, report)
        mapped_report['attachment_id'] = report['attachment_id']
        mapped_report['universal_unique_identifier'] = report['universal_unique_identifier']
        mapped_reports.append(mapped_report)
        
    jira_integration.create_Jira_tickets(mapped_reports)     

    return {'status': 'SUCCESS'}

def map_embedded_to_actual(map, report):
    result = {}    
    for attr1 in map:
        value = map[attr1]    
        if not value:
            continue;
    
        if isinstance(value, list):            
            replace_array_embedded_entries(value, report)        
        elif isinstance(value, dict):            
            replace_object_embedded_entries(value, report)
        else:            
            result = replace_primitive_embedded_entries(value, report)
            if result:
                value = map[attr1] = result            
                      
def replace_array_embedded_entries(value, report):        
    for entry in value:  
        replace_object_embedded_entries(entry, report)        

def replace_object_embedded_entries(entry, report):     
    for key in entry:        
        if entry[key] is None:
            continue

        result = replace_primitive_embedded_entries(entry[key], report)
        if result:
            entry[key] = result        

def replace_primitive_embedded_entries(value, report):    
    embedded_entries = get_embedded_entries(value)    
                    
    if embedded_entries is None:
        return

    for match in embedded_entries:        
        search_item = match[1:-1]        
        found = report.get(search_item, None)
        if found:            
            value = value.replace(match, str(report[search_item]))
    return value
            
def get_embedded_entries(value):
    if not isinstance(value, unicode):        
        return None    
    match = re.findall(EMBEDDED_MAP_ENTRY_PATTERN, value, re.M | re.I)    
    return match
