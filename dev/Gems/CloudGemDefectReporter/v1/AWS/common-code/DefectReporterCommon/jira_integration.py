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

from boto3.dynamodb.conditions import Key
from errors import ClientError
from cgf_utils import custom_resource_utils
from jira import JIRA, JIRAError
from defect_reporter_cloudwatch import CloudWatch
from botocore.client import Config
import CloudCanvas
import boto3
import json
import StringIO
import defect_reporter_constants as constants
import additonal_report_Info
import re
import copy
import os
import datetime

STANDARD_FIELD_TYPES = {'number': [int, long, float, complex], 'boolean': [bool], 'text': [str, unicode], 'object': [dict]}
CREDENTIAL_KEYS = ['userName', 'password', 'server']
EMBEDDED_MAP_ENTRY_PATTERN = "\[\s*\w*\s*\]"

def jira_credentials_status():
    jira_credentials = __get_credentials()

    credentials_exist = True
    for key in CREDENTIAL_KEYS:
        if jira_credentials[key] == '':
            credentials_exist = False
            break

    return { 'exist': credentials_exist, 'server': jira_credentials.get('server', '') }

def update_credentials(credentials):
    request = credentials
    request['kms_key'] = custom_resource_utils.get_embedded_physical_id(CloudCanvas.get_setting('KmsKey'))
    request['method'] = 'PUT'

    client = boto3.client('lambda')
    response = client.invoke(
        FunctionName = custom_resource_utils.get_embedded_physical_id(CloudCanvas.get_setting(constants.JIRA_CREDENTIALS_LAMBDA)),
        Payload = json.dumps(request)
    )

    return json.loads(response['Payload'].read().decode("utf-8"))

def get_jira_integration_settings():
    settings = {}
    settings['submitMode'] =__get_existing_mapping('submitMode')
    settings['project'] =__get_existing_mapping('project')
    settings['issuetype'] =__get_existing_mapping('issuetype')
    settings['projectAttributeSource'] =__get_existing_mapping('projectAttributeSource')
    settings['projectDefault'] =__get_existing_mapping('projectDefault')

    return settings

def get_project_keys():
    jira_client = get_jira_client()
    meta = jira_client.createmeta()

    projects = meta.get('projects', [])
    project_keys = [project.get('key', '') for project in projects]

    return project_keys

def get_issue_types(project_key):
    jira_client = get_jira_client()
    # Filter the meta data by project key
    meta = jira_client.createmeta(projectKeys = project_key)

    projects = meta.get('projects', [])
    if len(projects) == 1:
        issue_types = [issue_type.get('name', '') for issue_type in projects[0].get('issuetypes', [])]
        return issue_types
    else:
        raise ClientError("Invalid project key {}".format(project_key))

def get_field_mappings(project_key, issue_type):
    # Get the fields and their display names required by the selected project key and issue type
    fields = __get_jira_fields(project_key, issue_type)
    existing_fields = __get_existing_mapping(project_key) or {}
    existing_fields_issue_type = existing_fields.get(issue_type, {})    
    field_mappings = []
    for field in fields:
        mapping = existing_fields_issue_type.get(field['id'], {}).get('mapping', '')        
        field.update({'mapping': mapping})
        field_mappings.append(field)    
    return field_mappings


def get_field_mappings_dictionary(project, issuetype):
    field_mappings = get_field_mappings(project, issuetype)
    result = {}
    for entry in field_mappings:        
        result[entry['id']] = entry['mapping']
    return result

def update_field_mappings(mappings):
    __clear_cache()
    for item in mappings:
        #merge the project settings.  A project could have settings set for multiple different Jira issue types
        if item.get('name', None) == "Project" and item.get('id', None) and isinstance(item.get('mapping', None), dict):
            project_issue_types =  __get_existing_mapping( item.get('id') ) or {}
            item_mappings = item.get('mapping')  
            for key,value in item_mappings.iteritems():
                project_issue_types[key] = value
            item['mapping'] = project_issue_types            
            
        if item.get('mapping'):
            __get_jira_integration_settings_table().put_item(Item=item)
        elif item.get('mapping', None) != None and item.get('id'):
            key = {'id': item.get('id')}
            __get_jira_integration_settings_table().delete_item(Key=key)

    return 'SUCCESS'

def map_embedded_to_actual(map, report):
    keys_to_delete = []        
    for attr1 in map:
        map_value = map[attr1]        
        if not map_value:
            if attr1 in report:                
                map[attr1] = report[attr1]
            else:
                keys_to_delete.append(attr1)
            continue;               

        if isinstance(map_value, list):            
            replace_array_embedded_entries(map_value, report)        
        elif isinstance(map_value, dict):            
            replace_object_embedded_entries(map_value, report)
        else:            
            primitive_replace_result = replace_primitive_embedded_entries(map_value, report)            
            if primitive_replace_result:
                map[attr1] = primitive_replace_result

    for key in keys_to_delete:
        del map[key]
                      
def replace_array_embedded_entries(map_value, report):        
    for entry in map_value:  
        replace_object_embedded_entries(entry, report)        

def replace_object_embedded_entries(entry, report):     
    for key in entry:        
        if entry[key] is None:
            continue

        result = replace_primitive_embedded_entries(entry[key], report)
        if result:
            entry[key] = result        

def replace_primitive_embedded_entries(map_value, report):    
    embedded_entries = get_embedded_entries(map_value)    

    if embedded_entries is None:
        return
    
    for match in embedded_entries:
        search_item = match[1:-1]
        found = report.get(search_item, None)        
        if found:
            map_value = map_value.replace(match, str(report[search_item]))
    return map_value
            
def get_embedded_entries(map_value):
    if not isinstance(map_value, unicode):        
        return None    
    match = re.findall(EMBEDDED_MAP_ENTRY_PATTERN, map_value, re.M | re.I)    
    return match

def prepare_jira_tickets(reports, jira_integration_settings):
    project_attribute_source = jira_integration_settings.get('projectAttributeSource', None)      
    mapped_reports = []
    for report in reports:        
        route_name = report.get(project_attribute_source, '')
        project = get_project_name_from_report(jira_integration_settings, route_name.upper())
        field_mappings = get_field_mappings_dictionary(project, jira_integration_settings['issuetype'])
        mapped_report = copy.deepcopy(field_mappings)
        map_embedded_to_actual(mapped_report, report)
        mapped_report['attachment_id'] = json.loads(report['attachment_id']) if isinstance(report['attachment_id'], unicode) else report['attachment_id']
        mapped_report['universal_unique_identifier'] = report['universal_unique_identifier']
        mapped_report['project'] = report.get('project', None) or project
        mapped_report['issuetype'] = report.get('issuetype', None) or jira_integration_settings['issuetype']
        mapped_reports.append(mapped_report)
    return mapped_reports      

def create_Jira_tickets(prepared_reports):
    cw = CloudWatch()
    for report in prepared_reports:
        # Check whether the report is duplicated
        client = boto3.client('lambda')
        response = client.invoke(
            FunctionName = custom_resource_utils.get_embedded_physical_id(CloudCanvas.get_setting('DeduppingLambda')),
            Payload = json.dumps({'report': report})
        )
        issue_id = json.loads(response['Payload'].read().decode("utf-8"))
        attachments = report.get('attachment_id', None)
        identifier = report.get('universal_unique_identifier', None)
        
        report.pop('attachment_id')
        report.pop('universal_unique_identifier')  
        # Create a new JIRA ticket if the report is not duplicated

        if not issue_id:
            issue_id = __create_jira_ticket(cw, report, attachments)            

        update_occurance_count(issue_id)
        report['jira_status'] = issue_id        
        additonal_report_Info.update_report_header({
            'universal_unique_identifier': identifier,
            'jira_status': report.get('jira_status', None),
            'bookmark': report.get('bookmark', None),
            'report_status': report.get('report_status', None)
        })
    return 'SUCCESS'

def get_project_name_from_report(settings, jira_team_name):
    project = settings.get('projectDefault', None) or settings.get('project', None)
    if jira_team_name and __get_existing_mapping(jira_team_name) != '':
        project = jira_team_name       
    return project

def update_occurance_count(issue_id):
    if issue_id == None:
        return

    key = {'issue_id': issue_id}
    try:
        item = __get_jira_ticket_occurance_count_table().get_item(Key=key).get('Item', {})
        occurance_count = item.get('occurance_count', 0) + 1
    except ClientError as e:
        occurance_count = 1

    entry = {'issue_id': issue_id, 'occurance_count': occurance_count}    
    __get_jira_ticket_occurance_count_table().put_item(Item=entry)

def get_jira_client():
    jira_credentials = __get_credentials()
    jira_options = {'server':jira_credentials.get('server', '')}
    jira_client = JIRA(options=jira_options, basic_auth=(jira_credentials.get('userName', ''),jira_credentials.get('password', '')))

    return jira_client

def __get_credentials():
    request = {'method': 'GET'}

    client = boto3.client('lambda')
    response = client.invoke(
        FunctionName = custom_resource_utils.get_embedded_physical_id(CloudCanvas.get_setting(constants.JIRA_CREDENTIALS_LAMBDA)),
        Payload = json.dumps(request)
    )

    return json.loads(response['Payload'].read().decode("utf-8"))

jira_field_meta = {}
def __get_jira_fields(project_key, issue_type):    
    global jira_field_meta
    if project_key in jira_field_meta and issue_type in jira_field_meta[project_key]:
        return jira_field_meta[project_key][issue_type]   

    jira_client = get_jira_client()
    # Filter the meta data by project key and issue type name
    meta = jira_client.createmeta(projectKeys = project_key, issuetypeNames = issue_type, expand='projects.issuetypes.fields')

    # Retrieve the issue type description
    projects = meta.get('projects', [])
    issue_types = []
    issue_type_description = {}

    if len(projects) == 1:
        issue_types = projects[0].get('issuetypes', [])
    else:
        raise ClientError("Invalid project key {}".format(project_key))

    if len(issue_types) == 1:
        issue_type_description = issue_types[0]
    else:
        raise ClientError("Invalid issue type {} ".format(issue_type))

    fields = []
    for field_id, field_property in issue_type_description.get('fields', {}).iteritems():
        if field_id != 'project' and field_id != 'issuetype':
            fields.append({
                'id': field_id,
                'name': field_property.get('name', field_id),
                'required': field_property.get('required', True),
                'schema': __get_field_schema(field_property)
            })

    jira_field_meta[project_key] = {
        issue_type : fields
    }
    return fields

def __get_field_schema(field):
    allowed_value_schema = {}
    for allow_value in field.get("allowedValues", []):
        allowed_value_schema = __get_allowed_value_schema(allow_value, allowed_value_schema)
    
    field_type = field.get('schema', {}).get('type', '')
    field_type = 'text' if field_type == 'string' else field_type
    schema = {'type': field_type}

    if field_type == 'array':
        schema['items'] = allowed_value_schema if allowed_value_schema else {'type': field['schema']['items']}
    else:
        schema = allowed_value_schema if allowed_value_schema else schema

    if schema.get('properties'):
        schema['type'] = 'object'

    return schema

def __get_allowed_value_schema(allow_value, allowed_value_schema):
    allowed_value_type = allowed_value_schema.get('type', __get_standard_data_type(allow_value))
    allowed_value_schema['type'] = allowed_value_type

    if allowed_value_type != 'object':
        return

    if allowed_value_schema.get('properties') == None:
        allowed_value_schema['properties'] = {}

    for key, value in allow_value.iteritems():
        allowed_value_schema['properties'][key] = {'type': __get_standard_data_type(value)}

    return allowed_value_schema

def __get_standard_data_type(value):
    value_type = type(value)

    for key, types in STANDARD_FIELD_TYPES.iteritems():
        if value_type in types:
            return key

    return 'text'

def __find_embedded_identifiers(mappings):
    
    if type(mappings) != str:        
        return []

    EMBEDDED_LOOKUP_KEYS_REGEX_PATTERN = "\"\[(\S)*\]\""
       
    return re.findall(EMBEDDED_LOOKUP_KEYS_REGEX_PATTERN, mappings, re.MULTILINE);    

def __replace_embedded(value, report):
    embedded_mappings = __find_embedded_identifiers(value)    
    #replace the embedded mappings with the proper defect event attribute
    for embedded_map in embedded_mappings:
        defect_event_value = report.get(embedded_map, None)
        if defect_event_value:            
            value.replace(embedded_map, defect_event_value)

def __create_jira_ticket(cw, issue, attachments):                         
    jira_issue_key = __send_jira_ticket(cw, issue)
    __send_jira_attachments(jira_issue_key, attachments)


def __send_jira_ticket(cw, issue):        
    print "Sending ticket", issue
    # Create a new Jira ticket
    new_issue = {}

    try:
        new_issue  = get_jira_client().create_issue(**issue)
        __write_cloud_watch_metric(cw,"Success")
    except JIRAError as e:
        __write_cloud_watch_metric(cw,"Failure")
        raise ClientError(e.text)

    print "Jira ticket number {}".format(new_issue) 
    return new_issue.key

def __send_jira_attachments(jira_issue_key, attachments):
    # Upload the attachments
    print "Sending attachments for issue key '{}'".format(jira_issue_key), attachments
    for attachment in attachments:        
        __upload_attachment(attachment, jira_issue_key)    

def __upload_attachment(attachment, jira_issue):
    s3_client = boto3.client('s3', config=Config(signature_version='s3v4'))
    key = attachment.get('id', '')
    try:
        response = s3_client.get_object(Bucket = custom_resource_utils.get_embedded_physical_id(CloudCanvas.get_setting(constants.SANITIZED_BUCKET)), Key = key)
    except Exception as e:
        print "Unable to GET the sanitized attachment. Key==>", key
        return 

    new_attachment = StringIO.StringIO()
    new_attachment.write(response['Body'].read())

    attachment_object = get_jira_client().add_attachment(
        issue = jira_issue,
        attachment = new_attachment,
        filename = ('{}.{}').format(attachment.get('name', ''), attachment.get('extension', '')))

def __write_cloud_watch_metric(cw, status):
    namespace = "CloudCanvas/{}".format(os.environ['CloudCanvasServiceLambda'])    
    name = "JiraTicket"    
    metric = [{
                "MetricName": 'Processed',
                "Dimensions":[{'Name': name, 'Value': status}],
                "Timestamp":datetime.datetime.utcnow(),
                "Value": 1,                                
                "Unit": 'Count'                             
            }]    
    cw.put_metric_data(namespace,metric);

jira_mappings = {}
def __get_existing_mapping(id):
    global jira_mappings
    if id in jira_mappings:
        return jira_mappings[id]

    response = __get_jira_integration_settings_table().query(KeyConditionExpression=Key('id').eq(id))
    if len(response.get('Items', [])) > 0:
        mapping = response['Items'][0]['mapping']
        jira_mappings[id] = mapping
        return mapping
    return ''

def __clear_cache():
    global jira_mappings
    global jira_field_meta
    jira_mappings = {}
    jira_field_meta = {}

def __get_jira_integration_settings_table():
    if not hasattr(__get_jira_integration_settings_table,'jira_integration_settings'):
        __get_jira_integration_settings_table.jira_integration_settings = boto3.resource('dynamodb').Table(custom_resource_utils.get_embedded_physical_id(CloudCanvas.get_setting('JiraIntegrationSettings')))
    return __get_jira_integration_settings_table.jira_integration_settings

def __get_jira_ticket_occurance_count_table():
    if not hasattr(__get_jira_ticket_occurance_count_table,'jira_ticket_occurance_count'):
        __get_jira_ticket_occurance_count_table.jira_ticket_occurance_count = boto3.resource('dynamodb').Table(custom_resource_utils.get_embedded_physical_id(CloudCanvas.get_setting('JiraTicketOccuranceCount')))
    return __get_jira_ticket_occurance_count_table.jira_ticket_occurance_count