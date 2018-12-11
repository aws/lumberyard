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

import boto3
from boto3.dynamodb.conditions import Key
import CloudCanvas
from errors import ClientError
import json
import StringIO
import defect_reporter_constants as constants
from cgf_utils import custom_resource_utils
import additonal_report_Info
import re
from jira import JIRA, JIRAError

STANDARD_FIELD_TYPES = {'number': [int, long, float, complex], 'boolean': [bool], 'text': [str, unicode], 'object': [dict]}
CREDENTIAL_KEYS = ['userName', 'password', 'server']

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

    field_mappings = []
    for field in fields:
        field.update({'mapping': __get_existing_mapping(field['id'])})
        field_mappings.append(field)

    return field_mappings

def update_field_mappings(mappings):
    for item in mappings:
        if item.get('mapping'):
            __get_jira_integration_settings_table().put_item(Item=item)
        elif item.get('mapping', None) != None and item.get('id'):
            key = {'id': item.get('id')}
            __get_jira_integration_settings_table().delete_item(Key=key)

    return 'SUCCESS'

def create_Jira_tickets(reports, is_manual=False):
    settings = get_jira_integration_settings()
    if not (settings.get('project', '') and settings.get('issuetype', '')):
        raise ClientError('The project key or issue type is not specified.')

    # Get the args for creating a new Jira ticket based on the mappings defined in CGP
    issue_args = {'project': settings['project'], 'issuetype': settings['issuetype']}
    field_mappings = get_field_mappings(settings['project'], settings['issuetype'])

    for report in reports:
        # Check whether the report is duplicated
        client = boto3.client('lambda')
        response = client.invoke(
            FunctionName = custom_resource_utils.get_embedded_physical_id(CloudCanvas.get_setting('DeduppingLambda')),
            Payload = json.dumps({'report': report})
        )
        issue_id = json.loads(response['Payload'].read().decode("utf-8"))

        # Create a new JIRA ticket if the report is not duplicated
        if not issue_id:
            issue_id = __create_jira_ticket(report, field_mappings, issue_args, is_manual)

        update_occurance_count(issue_id)
        report['jira_status'] = issue_id

        additonal_report_Info.update_report_header(report)
    return 'SUCCESS'

def upload_attachment(attachment, jira_issue):
    s3_client = boto3.client('s3')
    key = attachment.get('id', '')
    try:
        response = s3_client.get_object(Bucket = custom_resource_utils.get_embedded_physical_id(CloudCanvas.get_setting(constants.SANITIZED_BUCKET)), Key = key)
    except Exception as e:
        print "Unable to GET the sanitized attachment. Key==>", key
        return 

    new_attachment = StringIO.StringIO()
    new_attachment.write(response['Body'].read())

    attachment_object = get_jira_client().add_attachment(
        issue = jira_issue.key,
        attachment = new_attachment,
        filename = ('{}.{}').format(attachment.get('name', ''), attachment.get('extension', '')))

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

def __get_jira_fields(project_key, issue_type):
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

def __create_jira_ticket(report, field_mappings, issue_args, is_manual_mode):        
    attachments = report.get('attachment_id', None)        

    issue_args.update(report)
            
    return __send_jira_ticket(issue_args, attachments)

def __replace_embedded(value, report):
    embedded_mappings = __find_embedded_identifiers(value)    
    #replace the embedded mappings with the proper defect event attribute
    for embedded_map in embedded_mappings:
        defect_event_value = report.get(embedded_map, None)
        if defect_event_value:            
            value.replace(embedded_map, defect_event_value)

def __send_jira_ticket(issue_args, attachments):    
    issue_args.pop('attachment_id')
    issue_args.pop('universal_unique_identifier')
    print "Sending ticket", issue_args, attachments
    # Create a new Jira ticket
    new_issue = {}
    try:
        new_issue  = get_jira_client().create_issue(**issue_args)
    except JIRAError as e:
        raise ClientError(e.text)

    # Upload the attachments
    for attachment in attachments:        
        upload_attachment(attachment, new_issue)

    return new_issue.key

def __get_existing_mapping(id):
    response = __get_jira_integration_settings_table().query(KeyConditionExpression=Key('id').eq(id))
    if len(response.get('Items', [])) > 0:
        return response['Items'][0]['mapping']
    return ''

def __get_jira_integration_settings_table():
    if not hasattr(__get_jira_integration_settings_table,'jira_integration_settings'):
        __get_jira_integration_settings_table.jira_integration_settings = boto3.resource('dynamodb').Table(custom_resource_utils.get_embedded_physical_id(CloudCanvas.get_setting('JiraIntegrationSettings')))
    return __get_jira_integration_settings_table.jira_integration_settings

def __get_jira_ticket_occurance_count_table():
    if not hasattr(__get_jira_ticket_occurance_count_table,'jira_ticket_occurance_count'):
        __get_jira_ticket_occurance_count_table.jira_ticket_occurance_count = boto3.resource('dynamodb').Table(custom_resource_utils.get_embedded_physical_id(CloudCanvas.get_setting('JiraTicketOccuranceCount')))
    return __get_jira_ticket_occurance_count_table.jira_ticket_occurance_count