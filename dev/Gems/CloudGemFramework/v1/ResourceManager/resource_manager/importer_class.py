import boto3
import json
import urllib
import zipfile
import StringIO
import os
import util

from errors import HandledError
from botocore.exceptions import ClientError

class ResourceImporter():
    def __init__(self, type, region, context):
        #A low-level client representing the resource
        if type != 's3':
            self.client = context.aws.client(type, region = region)           
        else:
            self.client = context.aws.client(type)

        self.region = region
        self.templates = []
        self.type = type
        self.function_accesses = []

    def list_resources(self, context):
        raise NotImplementedError

    def generate_templates(self, arn, resource_name, resource_group, context):
        raise NotImplementedError

    def add_resource(self, resource_name, resource_group, arn, download, context):
        #Check whether the resource with the same name exists
        group = context.resource_groups.get(resource_group) # raises HandledError if it doesn't exist
        data = group.template
        if resource_name in data['Resources'].keys():
            raise HandledError('Name {} has been used for another resource in the resource group.'.format(resource_name))
        #Generate the template for the resource
        self.templates = self.generate_templates(arn, resource_name, resource_group, context)
        #Choose the related function actions and add them to the function_access
        function_access_list = []
        for function_access in self.function_accesses:          
            if self.type + ':*' in function_access['actions']:
                function_access_list.append({'Action': self.type + ':*', 'FunctionName': function_access['function_name'], "ResourceSuffix": "*"})
            else:
                for action in function_access['actions']:  
                    if action.split(':')[0] == self.type:
                        function_access_list.append({'Action': action, 'FunctionName': function_access['function_name'], "ResourceSuffix": "*"})
        #Generate the CloudCanvas Metadata
        if function_access_list:
            self.templates[-1][resource_name]['Metadata'] = {'CloudCanvas': {'FunctionAccess': function_access_list}}
        #Add the template to the resource group
        for template in self.templates:
            group = context.resource_groups.get(resource_group)
            data = group.template
            data['Resources'].update(template)
            context.config.save_resource_group_template(resource_group)
            if (template.keys()[0] != resource_name):
                context.view.import_resource(template.keys()[0])
        #DynamoDB table and SQS queue don't need to add related resources. Import successfully.
        if self.type in ['dynamodb','sqs']:
            context.view.import_resource(resource_name)

class DynamoDBImporter(ResourceImporter):
    def __init__(self, region, context):
        ResourceImporter.__init__(self,'dynamodb', region, context)
        self.table_list = self.client.list_tables()

    def list_resources(self, context):       
        #Print the resource name and its ARN
        importable_resources = []
        for table in self.table_list.get('TableNames',[]):
            description = self.client.describe_table(TableName = table)
            importable_resources.append(
            {
                'Name': table,
                'ARN': description['Table']['TableArn']
            })
        context.view.importable_resource_list(importable_resources)

    def generate_templates(self, arn, resource_name, resource_group, context):
        #Check whether the resource is a DynamoDB table
        if arn.find('table/') == -1:
            raise HandledError('Types of the ARN and the resource do not match.')
        original_name = arn.split('/')[-1]
        #Get the description of the table
        if original_name not in self.table_list['TableNames']:
            raise HandledError('Resource with ARN {} does not exist.'.format(arn))
        description = self.client.describe_table(TableName = original_name).get('Table',{})
        #generate the template for the resource 
        output = {}
        output['Type'] = 'AWS::DynamoDB::Table'
        output['Properties'] = {}
        output['Properties']['AttributeDefinitions'] = description.get('AttributeDefinitions')
        #GlobalSecondaryIndexes is not required
        global_secondary_indexes = []
        for index in description.get('GlobalSecondaryIndexes', []):
            global_secondary_index = {k: index.get(k) for k in ('IndexName', 'KeySchema', 'Projection', 'ProvisionedThroughput')}
            if global_secondary_index.get('ProvisionedThroughput'):
                global_secondary_index['ProvisionedThroughput'].pop('LastIncreaseDateTime', None)
                global_secondary_index['ProvisionedThroughput'].pop('LastDecreaseDateTime', None)
                global_secondary_index['ProvisionedThroughput'].pop('NumberOfDecreasesToday', None)
            global_secondary_indexes.append(global_secondary_index)
        if global_secondary_indexes:
            output['Properties']['GlobalSecondaryIndexes'] = global_secondary_indexes
        output['Properties']['KeySchema'] = description.get('KeySchema')
        #LocalSecondaryIndexes is not required
        local_secondary_indexes = []
        for index in description.get('LocalSecondaryIndexes', []):
            local_secondary_indexes.append(
                {k: index.get(k) for k in ('IndexName', 'KeySchema', 'Projection')})
        if local_secondary_indexes:
            output['Properties']['LocalSecondaryIndexes'] = local_secondary_indexes
        output['Properties']['ProvisionedThroughput'] = {k: description.get('ProvisionedThroughput').get(k) 
            for k in ('ReadCapacityUnits', 'WriteCapacityUnits')}
        #StreamSpecification is not required
        if description.get('StreamSpecification',{}).get('StreamViewType'):
            output['Properties']['StreamSpecification'] = {'StreamViewType': description.get('StreamSpecification').get('StreamViewType')}
        self.templates.append({resource_name: output})
        return self.templates

class S3Importer(ResourceImporter):
    def __init__(self, region, context):
        ResourceImporter.__init__(self,'s3', region, context)
        self.bucket_list = [bucket['Name'] for bucket in self.client.list_buckets()['Buckets']]
        #Use a list to record the dependency with lambda function
        self.lambda_dependencies = []
        #Use a list to record the dependency with sns topic
        self.topic_dependencies = []
        #Use a list to record the dependency with sqs queue
        self.queue_dependencies = []

    def list_resources(self, context):
        #Print the resource name and its ARN
        importable_resources = []
        for bucket in self.bucket_list:
            importable_resources.append(
            {
                'Name': bucket,
                'ARN': 'arn:aws:s3:::' + bucket
            })
        context.view.importable_resource_list(importable_resources)

    def generate_templates(self, arn, resource_name, resource_group, context):
        #Check whether the resource is an S3 bucket
        if arn.find('s3:::') == -1:
            raise HandledError('Types of the ARN and the resource do not match.')
        #Check whether the bucket exists
        bucket_name = arn.split(':')[-1]
        bucket_list = self.client.list_buckets()
        if bucket_name not in self.bucket_list:
            raise HandledError('Resource with ARN {} does not exist.'.format(arn))
        #Generate the template for the resource 
        output = {}
        output['Type'] = 'AWS::S3::Bucket'
        output['Properties'] = {}
        #CorsConfiguration is not required
        cors_configuration = self._get_cors_configuration(bucket_name)
        if cors_configuration.get('CorsRules'):
            output['Properties']['CorsConfiguration'] = cors_configuration
        #LifecycleConfiguration is not required
        lifecycle_configuration =  self._get_lifecycle_configuration(bucket_name)
        if lifecycle_configuration.get('Rules'):
            output['Properties']['LifecycleConfiguration'] = lifecycle_configuration 
        #NotificationConfiguration is not required                     
        notification_configuration = self._get_notification_configuration(bucket_name, resource_name, context)
        if notification_configuration:
            output['Properties']['NotificationConfiguration'] = notification_configuration                   
        #tags is not required
        tags = self._get_tags(bucket_name)
        if tags:
            output['Properties']['Tags'] = tags
        #VersioningConfiguration is not required
        VersioningConfiguration = {'Status': self.client.get_bucket_versioning(
            Bucket = bucket_name).get('Status', '')}
        if VersioningConfiguration['Status']:
            output['Properties']['VersioningConfiguration'] = VersioningConfiguration        
        #WebsiteConfiguration is not required
        website_configuration = self._get_website_configuration(bucket_name)
        if website_configuration:
            output['Properties']['WebsiteConfiguration'] = website_configuration
        #Add the resource Dependency
        depends_on = []
        for lambda_dependency in self.lambda_dependencies:
            depends_on.extend([lambda_dependency['name'], lambda_dependency['name'] + 'Configuration', lambda_dependency['name'] + 'Permission'])
        for topic_dependency in self.topic_dependencies:
            depends_on.extend([topic_dependency['name'], topic_dependency['name'] + 'Permission'])
        for queue_dependency in self.queue_dependencies:
            depends_on.extend([queue_dependency['name'], queue_dependency['name'] + 'Permission'])
        if depends_on:
            output['DependsOn'] = depends_on
        self.templates.append({resource_name: output})
        return self.templates

    def _get_cors_configuration(self, bucket_name):
        try:
            cors_configuration = {'CorsRules': self.client.get_bucket_cors(
                Bucket = bucket_name).get('CORSRules', [])}
        except ClientError as e:
            if e.response['Error']['Code'] == 'NoSuchCORSConfiguration':
                return {}
            else:
                raise e
        for rule in cors_configuration.get('CorsRules', []):
            if rule.get('MaxAgeSeconds'):
                rule['MaxAge'] = rule.get('MaxAgeSeconds')
            rule.pop('MaxAgeSeconds', None)
        return cors_configuration

    def _get_lifecycle_configuration(self, bucket_name):
        try:
            lifecycle_configuration = {'Rules': self.client.get_bucket_lifecycle_configuration(
                Bucket = bucket_name).get('Rules', [])}
        except ClientError as e:
            if e.response['Error']['Code'] == 'NoSuchLifecycleConfiguration':
                return {}
            else:
                raise e  
        for rule in lifecycle_configuration.get('Rules',[]):
            if rule.get('ID'):
                rule['Id'] = rule.get('ID')
            rule.pop('ID', None)
            if rule.get('Expiration', {}).get('Date'):          
                rule['ExpirationDate'] = rule.get('Expiration').get('Date')
            if rule.get('Expiration', {}).get('Days'): 
                rule['ExpirationInDays'] = rule.get('Expiration').get('Days')
            if rule.get('NoncurrentVersionExpiration', {}).get('NoncurrentDays'): 
                rule['NoncurrentVersionExpirationInDays'] = rule.get('NoncurrentVersionExpiration').get('NoncurrentDays')
            rule.pop('Expiration', None) 
            rule.pop('NoncurrentVersionExpiration', None)
            rule.pop('AbortIncompleteMultipartUpload', None)
            for transition in rule.get('Transitions', {}):
                if transition.get('Date'): 
                    transition['TransitionDate'] = transition.get('Date')
                if transition.get('Days'): 
                    transition['TransitionInDays'] = transition.get('Days')
                transition.pop('Date', None) 
                transition.pop('Days', None)
        return lifecycle_configuration
    
    def _get_notification_configuration(self, bucket_name, resource_name, context):
        notification = self.client.get_bucket_notification_configuration(
            Bucket = bucket_name)
        notification_configuration = {}
        #Deal with LambdaFunctionConfigurations
        notification_configuration_detail = self._get_notification_configuration_detail('LambdaFunction', resource_name, notification)
        lambda_configurations = notification_configuration_detail['configurations']
        self.lambda_dependencies = notification_configuration_detail['dependencies']
        #Get the permissions of the related lambda functions
        for lambda_function in self.lambda_dependencies:            
            region = util.get_region_from_arn(lambda_function['arn'])
            lambda_importer = LambdaImporter(region, context)
            original_name = lambda_function['arn'].split(':')[-1]
            self.function_accesses.append(lambda_importer.get_permissions(original_name, lambda_function['name'], context, region))
        if lambda_configurations:
            notification_configuration['LambdaConfigurations'] = lambda_configurations

        #Deal with TopicConfigurations
        notification_configuration_detail = self._get_notification_configuration_detail('Topic', resource_name, notification)
        topic_configurations = notification_configuration_detail['configurations']
        self.topic_dependencies = notification_configuration_detail['dependencies']
        if topic_configurations:
            notification_configuration['TopicConfigurations'] = topic_configurations

        #Deal with QueueConfigurations
        notification_configuration_detail = self._get_notification_configuration_detail('Queue', resource_name, notification)
        queue_configurations = notification_configuration_detail['configurations']
        self.queue_dependencies = notification_configuration_detail['dependencies']
        if queue_configurations:
            notification_configuration['QueueConfigurations'] = queue_configurations

        return notification_configuration

    def _get_notification_configuration_detail(self, service_type, resource_name, notification):
        service_configurations = []
        service_dependencies = []
        count = 0
        if service_type == 'LambdaFunction':
            service_type_name = 'Function'
        else:
            service_type_name = service_type
        for service in notification.get(service_type + 'Configurations', []):
            #Record the dependency with the service
            service_arn = service[service_type + 'Arn']
            service_name = resource_name + 'AutoAdded' + service_type + str(count)
            service_dependencies.append({'arn': service_arn, 'name': service_name})
            #Generate the service configuration
            for event in service['Events']:
                if service_type == 'Topic':
                    service_type_name_detail = {"Ref": service_name}
                else:
                    service_type_name_detail = {"Fn::GetAtt" : [service_name, "Arn" ]}

                if service.get('Filter'):
                    configurations.append(
                        {'Event': event, 'Filter': service.get('Filter'), service_type_name: service_type_name_detail})
                else:
                    service_configurations.append(
                    {'Event': event, service_type_name: service_type_name_detail})
            count = count + 1
        notification_configuration_detail = {'configurations': service_configurations, 'dependencies': service_dependencies}
        return notification_configuration_detail

    def _get_website_configuration(self, bucket_name):
        try:
            website_configuration = self.client.get_bucket_website(
                Bucket = bucket_name)
        except ClientError as e:
            if e.response['Error']['Code'] == 'NoSuchWebsiteConfiguration':
                return {}
            else:
                raise e
        website_configuration.pop('ResponseMetadata', None)
        if website_configuration.get('IndexDocument', {}).get('Suffix'):
            website_configuration['IndexDocument'] = website_configuration.get('IndexDocument').get('Suffix')
        if website_configuration.get('ErrorDocument', {}).get('Key'):
            website_configuration['ErrorDocument'] = website_configuration.get('ErrorDocument').get('Key')
        for rule in website_configuration.get('RoutingRules', []):
            if rule.get('Redirect'):
                rule['RedirectRule'] = rule.get('Redirect')
            rule.pop('Redirect', None)
            if rule.get('Condition'):          
                rule['RoutingRuleCondition'] = rule.get('Condition')
            rule.pop('Condition', None)
        return website_configuration

    def _get_tags(self, bucket_name):
        try:
            tags = self.client.get_bucket_tagging(Bucket = bucket_name).get('TagSet')
        except ClientError as e:
            if e.response['Error']['Code'] == 'NoSuchTagSet':
                return []
            else:
                raise e
        valid_tags = []
        for tag in tags:
            if tag.get('Key').find('cloudformation') == -1:
                valid_tags.append(tag)
        return valid_tags

    def _generate_policy(self, resource_name, action, policy_type, resource_type):
        #Associates the resource with a policy to interact with S3 bucket
        policy = {}
        policy['Type'] = policy_type
        policy['Properties'] = {}
        if resource_type == 'Lambda':
            policy['Properties']['FunctionName'] = {'Fn::GetAtt': [resource_name, 'Arn']}
            policy['Properties']['Action'] = action
            policy['Properties']['Principal'] = 's3.amazonaws.com'
        else:
            policy_document = {
                "Statement": [
                    {
                        "Effect": "Allow",
                        "Principal": {
                            "Service": "s3.amazonaws.com"
                        },
                        "Action": action,
                        "Resource": "*"
                    }
                ]
            }
            policy['Properties']['PolicyDocument'] = policy_document
            policy['Properties'][resource_type] =[{"Ref": resource_name}]
        return policy

    def _generate_importer(self, type, region, context):
        if type == 'lambda':
            resource_importer = LambdaImporter(region, context)
        elif type == 'sqs':
            resource_importer = SQSImporter(region, context)
        elif type == 'sns':
            resource_importer = SNSImporter(region, context)
        return resource_importer

    def _import_related_resources(self, service_type, service_dependencies, resource_group, context):
        policy_arguments = {'sqs': {'policy_type': 'AWS::SQS::QueuePolicy', 'action': 'SQS:SendMessage', 'service_type': 'Queues'},
                          'sns': {'policy_type': 'AWS::SNS::TopicPolicy', 'action': 'SNS:Publish', 'service_type': 'Topics'},
                          'lambda': {'policy_type': 'AWS::Lambda::Permission', 'action': 'lambda:InvokeFunction', 'service_type': 'Lambda'}}
        for service_dependency in service_dependencies:
            #Create an importer according to the resource type
            region = util.get_region_from_arn(service_dependency['arn'])
            service_importer = self._generate_importer(service_type, region, context)
            #Generate the policy and add it to the template
            policy = self._generate_policy(service_dependency['name'], policy_arguments[service_type]['action'], 
                                            policy_arguments[service_type]['policy_type'], policy_arguments[service_type]['service_type'])
            service_importer.templates.append({service_dependency['name'] + 'Permission': policy})
            #Import the related resource to the resource group
            service_importer.add_resource(service_dependency['name'], resource_group, service_dependency['arn'], False, context)
            context.view.auto_added_resource(service_dependency['name'])

    def add_resource(self, resource_name, resource_group, arn, download, context):
        ResourceImporter.add_resource(self, resource_name, resource_group, arn, download, context)
        #Import the related AWS resources
        service_dependencies = {'lambda': self.lambda_dependencies, 'sqs': self.queue_dependencies, 'sns': self.topic_dependencies}
        for service_type in service_dependencies.keys():
            self._import_related_resources(service_type,service_dependencies[service_type], resource_group, context)       
        context.view.import_resource(resource_name)

        #Download the contents in the bucket if the user chooses to
        if not download:
            return
        else:
            bucket_name = arn.split(':')[-1]
            dir = os.path.join(context.config.base_resource_group_directory_path, resource_group)
            #Get all the objects in the bucket
            objects = self.client.list_objects(Bucket = bucket_name).get('Contents', [])
            for object in objects:
                #Create the necessary local directory
                string_list = object['Key'].split('/')
                if object['Key'].find(string_list[len(string_list) - 1]) != 0:
                    directory = dir + '/s3-bucket-content/' + resource_name + '/' + object['Key'][:object['Key'].find(string_list[len(string_list) - 1]) - 1]
                else:
                    directory = dir + '/s3-bucket-content/' + resource_name + '/'

                if not os.path.exists(directory):
                    os.makedirs(directory)

                if not object['Key'].endswith('/'): # the key is actually an empty dir
                    #Download the contents
                    self.client.download_file(bucket_name, object['Key'], directory + '/' + string_list[len(string_list) - 1])

class LambdaImporter(ResourceImporter):
    def __init__(self, region, context):
        ResourceImporter.__init__(self,'lambda', region, context)
        self.function_list = self.client.list_functions().get('Functions',[])
        #Use a list to record the dependency with DynamDB table
        self.table_dependencies = []
        
    def list_resources(self,context):
        #Print the resource name and its ARN
        importable_resources = []
        for function in self.function_list:
            importable_resources.append(
            {
                'Name': function['FunctionName'],
                'ARN': function['FunctionArn']
            })
        context.view.importable_resource_list(importable_resources)
    
    def get_permissions(self, original_name, resource_name, context, region):
        #Get the role of the lambda function
        description = self.client.get_function(FunctionName = original_name)['Configuration']
        role = description.get('Role').split('/')[-1]
        client = context.aws.client('iam', region = region)
        #Get the permissions according to the role
        role_policies = []
        for policy in client.list_role_policies(RoleName = role)['PolicyNames']:
            role_policies.append(client.get_role_policy(RoleName = role,PolicyName = policy))
        #Get all the possible actions
        actions = set()
        for role_policy in role_policies:
            for item in role_policy['PolicyDocument']['Statement']:
                actions = actions.union(set(item['Action']))
        permissions = {'function_name': resource_name, 'actions': list(actions)}
        return permissions

    def generate_templates(self, arn, resource_name, resource_group, context):
        #Check whether the resource is a Lambda function
        if arn.find('function:') == -1:
            raise HandledError('Types of the ARN and the resource do not match.')
        #Get the description of the function
        original_name = arn.split(':')[-1]
        if original_name not in [function['FunctionName'] for function in self.function_list]:
            raise HandledError('Resource with ARN {} does not exist.'.format(arn))
        description = self.client.get_function(FunctionName = original_name)['Configuration']
        #Generate the configuration
        output_configuration = self._generate_lambda_configuration(original_name, resource_name)
        #Generate the template for the resource
        output_template = {}
        output_template['Type'] = 'AWS::Lambda::Function'
        output_template['Properties'] = {}
        output_template['Properties']['Code'] = {}
        output_template['Properties']['Code']['S3Bucket'] = {'Fn::GetAtt': [resource_name + 'Configuration', 'ConfigurationBucket']}
        output_template['Properties']['Code']['S3Key'] = {'Fn::GetAtt': [resource_name + 'Configuration', 'ConfigurationKey']}
        #description is not required
        if description.get('Description'):
            output_template['Properties']['Description'] = description.get('Description')
        output_template['Properties']['Handler'] = description.get('Handler')
        #MemorySize is not required
        if description.get('MemorySize'):
            output_template['Properties']['MemorySize'] = description.get('MemorySize')
        output_template['Properties']['Role'] = {'Fn::GetAtt': [resource_name + 'Configuration', 'Role']}
        output_template['Properties']['Runtime'] = {'Fn::GetAtt': [resource_name + 'Configuration', 'Runtime']}
        #Timeout is not required
        if description.get('Timeout'):
            output_template['Properties']['Timeout'] = description.get('Timeout')
        #VpcConfig is not required
        if description.get('VpcConfig',{}).get('SecurityGroupIds') and description.get('VpcConfig',{}).get('SubnetIds'):
            output_template['Properties']['VpcConfig'] = {k: description.get('VpcConfig',{}).get(k,[]) 
                for k in ('SecurityGroupIds', 'SubnetIds')}
        self.templates.append({resource_name + 'Configuration': output_configuration})
        self.templates.append({resource_name: output_template})
        #Get the related DynamoDB table list
        self.table_dependencies = self._get_table_dependencies(original_name, resource_name)
        return self.templates

    def add_resource(self, resource_name, resource_group, arn, download, context):
        ResourceImporter.add_resource(self, resource_name, resource_group, arn, download, context)
        function_region = util.get_region_from_arn(arn)
        original_name = arn.split(':')[-1]
        for table_dependency in self.table_dependencies:
            dynamodb_region = util.get_region_from_arn(table_dependency['arn'])
            dynamodb_importer = DynamoDBImporter(dynamodb_region, context)
            dynamodb_importer.function_accesses.append(self.get_permissions(original_name, resource_name, context, function_region))
            #Add the event source mapping resource
            event_source = self._generate_table_event_source(resource_name, table_dependency['name'])
            dynamodb_importer.templates.append({table_dependency['name'] + 'EventSource': event_source})
            dynamodb_importer.add_resource(table_dependency['name'], resource_group, table_dependency['arn'], False, context)
            context.view.auto_added_resource(table_dependency['name'])      
        #Download the code
        dir = os.path.join(context.config.base_resource_group_directory_path, resource_group)
        description = self.client.get_function(FunctionName = arn.split(':')[-1])
        location = description['Code']['Location']
        zipcontent = urllib.urlopen(location)
        zfile = zipfile.ZipFile(StringIO.StringIO(zipcontent.read()))
        zfile.extractall(dir + '/lambda-function-code/')

        context.view.import_resource(resource_name)

    def _generate_lambda_configuration(self, original_name, resource_name):
        lambda_configuration = self.client.get_function(FunctionName = original_name)
        configuration = {}
        configuration['Properties'] = {}
        configuration['Properties']['ConfigurationBucket'] = {'Ref': 'ConfigurationBucket'}
        configuration['Properties']['ConfigurationKey'] = {'Ref': 'ConfigurationKey'}
        configuration['Properties']['FunctionName'] = resource_name
        configuration['Properties']['Runtime'] = lambda_configuration.get('Configuration').get('Runtime')
        configuration['Properties']['ServiceToken'] = {'Ref': 'ProjectResourceHandler'}
        configuration['Properties']['Settings'] = {}
        configuration['Type'] = 'Custom::LambdaConfiguration'
        return configuration

    def _get_table_dependencies(self, function_name, resource_name):
        table_dependencies = []
        event_source_mappings = self.client.list_event_source_mappings(FunctionName = function_name)
        count = 0
        #Find the related DynamoDB table from DynamoDBStream
        for event_source_mapping in event_source_mappings.get('EventSourceMappings'):
            stream_arn = event_source_mapping.get('EventSourceArn')
            table_arn = stream_arn[:stream_arn.find('/stream')]
            table_name = resource_name + 'AutoAddedtable' + str(count)
            table_dependencies.append({'arn': table_arn, 'name': table_name})
            count = count + 1
        return table_dependencies

    def _generate_table_event_source(self, resource_name, table_name):
        #Generate the event source mapping resource template
        event_source = {}
        event_source['Properties'] = {}
        event_source['Properties']['EventSourceArn'] = {"Fn::GetAtt": [table_name, "StreamArn"]}
        event_source['Properties']['FunctionName'] = resource_name
        #Set TRIM_HORIZON as the default value of StartingPosition
        event_source['Properties']['StartingPosition'] = 'TRIM_HORIZON'
        event_source['DependsOn'] = [resource_name, resource_name + 'Configuration', table_name]
        event_source['Type'] = 'AWS::Lambda::EventSourceMapping'
        return event_source

class SQSImporter(ResourceImporter):
    def __init__(self, region, context):
        ResourceImporter.__init__(self, 'sqs', region, context)
        self.queue_list = self.client.list_queues().get('QueueUrls', [])

    def list_resources(self, context):
        #Print the resource name and its ARN
        importable_resources = []
        for url in self.queue_list:
            account_id = url.split('/')[-2]
            resource_name = url.split('/')[-1]
            queue_arn = 'arn:aws:sqs:' + self.region + ':' + account_id + ':' + resource_name
            importable_resources.append(
            {
                'Name': url.split('/')[-1],
                'ARN': queue_arn
            })
        context.view.importable_resource_list(importable_resources)

    def generate_templates(self, arn, resource_name, resource_group, context):
        #Check whether the resource is an SQS queue
        if arn.find(':sqs:') == -1:
            raise HandledError('Types of the ARN and the resource do not match.')
        #Check whether the queue exists. If it exists, get its url.
        queue_name = arn.split(':')[-1]
        exist = False
        resource_url = ''
        for url in self.queue_list:
            if url.split('/')[-1] == queue_name:
                exist = True
                resource_url = url
                break
        if not exist:
            raise HandledError('Resource with ARN {} does not exist.'.format(arn))
        description = self.client.get_queue_attributes(QueueUrl = url, AttributeNames=['All'])['Attributes']
        #Generate the template for the resource
        output = {}
        output['Type'] = 'AWS::SQS::Queue'
        output['Properties'] = {}
        properties = ['DelaySeconds', 'MaximumMessageSize', 'MessageRetentionPeriod', 
                      'ReceiveMessageWaitTimeSeconds', 'RedrivePolicy', 'VisibilityTimeout']
        for property in properties:
            if description.get(property):
                output['Properties'][property] = description.get(property)
        self.templates.append({resource_name: output})
        return self.templates

class SNSImporter(ResourceImporter):
    def __init__(self, region, context):
        ResourceImporter.__init__(self, 'sns', region, context)
        self.queue_list = self.client.list_topics()
        #Use a list to record the dependency with lambda function
        self.lambda_dependencies = []

    def list_resources(self, context):
        #Print the resource name and its ARN
        importable_resources = []
        for topic in self.queue_list.get('Topics',[]):
            importable_resources.append(
            {
                'Name': topic['TopicArn'].split(':')[-1],
                'ARN': topic['TopicArn']
            })
        context.view.importable_resource_list(importable_resources)

    def generate_templates(self, arn, resource_name, resource_group, context):
        #Check whether the resource is an SNS topic
        if arn.find(':sns:') == -1:
            raise HandledError('Types of the ARN and the resource do not match.')
        #Check whether the topic exists.
        if {'TopicArn': arn} not in self.queue_list['Topics']:
            raise HandledError('Resource with ARN {} does not exist.'.format(arn))
        description = self.client.get_topic_attributes(TopicArn = arn)
        #Generate the template for the resource
        output = {}
        output['Type'] = 'AWS::SNS::Topic'
        output['Properties'] = {}
        #DisplayName is not required
        if description['Attributes'].get('DisplayName'):
            output['Properties']['DisplayName'] = description['Attributes'].get('DisplayName')
        #Subscription is not required
        subscriptions = []
        depends_on = []
        count = 0
        for subscription in self.client.list_subscriptions_by_topic(TopicArn = arn)['Subscriptions']:
            subscriptions.append({'Endpoint': subscription['Endpoint'], 'Protocol': subscription['Protocol']})
            #Get the related lambda functions
            if subscription['Protocol'] == 'lambda':
                function_name = resource_name + 'AutoAddedLambdaFunction' + str(count)
                self.lambda_dependencies.append({'arn': subscription['Endpoint'], 'name': function_name})
                depends_on.extend([function_name, function_name + 'Configuration'])
                region = util.get_region_from_arn(subscription['Endpoint'])
                original_name = subscription['Endpoint'].split(':')[-1]
                lambda_importer = LambdaImporter(region, context)
                self.function_accesses.append(lambda_importer.get_permissions(original_name, function_name, context, region))
                count = count + 1
        if subscriptions:
            output['Properties']['Subscription'] = subscriptions
        if depends_on:
            output['DependsOn'] = depends_on
        self.templates.append({resource_name: output})
        return self.templates

    def add_resource(self, resource_name, resource_group, arn, download, context):
        ResourceImporter.add_resource(self, resource_name, resource_group, arn, download, context)
        #Import the related Lambda functions
        for lambda_dependency in self.lambda_dependencies:
            region = util.get_region_from_arn(lambda_dependency['arn'])
            lambda_importer = LambdaImporter(region, context)
            lambda_importer.add_resource(lambda_dependency['name'], resource_group, lambda_dependency['arn'], False, context)
            context.view.auto_added_resource(lambda_dependency['name'])
        
        context.view.import_resource(resource_name)
