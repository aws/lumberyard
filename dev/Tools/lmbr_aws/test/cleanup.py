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
# $Revision: #2 $

import argparse
import boto3
import time
import os
import json
import sys
from botocore.client import Config
from botocore.exceptions import ClientError
from ConfigParser import RawConfigParser
from StringIO import StringIO

DEFAULT_REGION='us-east-1'

DEFAULT_PREFIXES = [ 'cctest' ]


def global_resource(cleanup_function):
    def wrapper(cleaner):
        if cleaner.delete_global_resources:
            cleanup_function(cleaner)
        else:
            print("Skipping deletion of global {} (use --delete-global-resources or exclude --region to delete)".
                format(cleanup_function.__name__[len("_delete_"):]))
    
    return wrapper

        
class Cleaner:

    def __init__(self, session, delete_global_resources):
        self.delete_global_resources = delete_global_resources
        self.cf = session.client('cloudformation')
        self.s3 = session.client('s3',config=Config(signature_version='s3v4'))
        self.iam = session.client('iam')
        self.cognito_identity = session.client('cognito-identity')
        self.cognito_idp = session.client('cognito-idp')
        self.logs = session.client('logs')
        self.apigateway = session.client('apigateway')
        self.dynamodb = session.client('dynamodb')
        self.lambda_client = session.client('lambda')        
        self.sqs = session.client('sqs')                
        self.glue = session.client('glue')
        self.iot_client = session.client('iot')
        self.swf = session.client('swf')
        
    def cleanup(self, prefixes, exceptions):
        self.__prefixes = prefixes
        self.__exceptions = exceptions
        self._delete_stacks()
        self._delete_identity_pools()
        self._delete_user_pools()        
        self._delete_dynamodb_tables()
        self._delete_log_groups()
        self._delete_api_gateway()
        self._delete_lambdas()
        self._delete_sqs_queues()
        self._delete_glue_crawlers()
        self._delete_glue_databases()
        self._delete_buckets()
        self._delete_swf_resources()

        #users/roles/policies need to be deleted last
        self._delete_users()
        self._delete_roles()
        self._delete_policies()
        self._delete_iot_policies()
        
    def _has_prefix(self, name):
        
        name = name.lower()

        for exception in self.__exceptions:
            if name.startswith(exception):
                return False

        for prefix in self.__prefixes:
            if name.startswith(prefix):
                return True

        return False

    def __describe_prefixes(self):
        return str(self.__prefixes) + ' but not ' + str(self.__exceptions)

    def _delete_glue_crawlers(self):
        if not self.glue:
            print 'No glue client available, skipping glue crawlers'
            return
			
        print '\n\nlooking for Glue crawlers with names starting with one of', self.__describe_prefixes()
        token = None
        while True:
            params = {}   
            if token:
                params["NextToken"]= token
            res = self.glue.get_crawlers(**params)   
            token = res.get('NextToken')
            for crawler in res['Crawlers']:
                crawler_name = crawler['Name']                
                if not self._has_prefix(crawler_name):                
                    continue
                print '  found crawler', crawler_name
                while True:
                    try:
                        print '    deleting crawler', crawler_name
                        self.glue.delete_crawler(
                                        Name=crawler_name
                                    )
                        break
                    except ClientError as e:
                        if e.response["Error"]["Code"] == "LimitExceededException":
                            print '    too many requests, sleeping...'                        
                            time.sleep(15)
                        else:
                            print '      ERROR:', e
                            break   
            if token is None:
                break

    def _delete_glue_tables(self, database_name):
        print '\tlooking for Glue tables with names starting with one of', self.__describe_prefixes()
        token = None
        while True:
            params = {}
            params['DatabaseName'] = database_name   
            if token:
                params["NextToken"]= token
            res = self.glue.get_tables(**params)   
            token = res.get('NextToken')
            for table in res['TableList']:
                table_name = table['Name']                
                if not self._has_prefix(table_name):                
                    continue
                print '  found glue table', table_name
                while True:
                    try:
                        print '\t\tdeleting glue table', table_name
                        self.glue.delete_table(
                                        Name=table_name,
                                        DatabaseName=database_name
                                    )
                        break
                    except ClientError as e:
                        if e.response["Error"]["Code"] == "LimitExceededException":
                            print '    too many requests, sleeping...'                        
                            time.sleep(15)
                        else:
                            print '      ERROR:', e
                            break   
            if token is None:
                break

    def _delete_glue_databases(self):
        if not self.glue:
            print 'No glue client available, skipping glue databases'
            return
			
        print '\n\nlooking for Glue databases with names starting with one of', self.__describe_prefixes()
        token = None
        while True:
            params = {}   
            if token:
                params["NextToken"]= token
            res = self.glue.get_databases(**params)   
            token = res.get('NextToken') 
            for database in res['DatabaseList']:
                database_name = database['Name']                
                if not self._has_prefix(database_name):                
                    continue
                print '  found glue database', database_name
                self._delete_glue_tables(database_name)
                while True:
                    try:
                        print '    deleting glue database', database_name
                        self.glue.delete_database(
                                        Name=database_name
                                    )
                        break
                    except ClientError as e:
                        if e.response["Error"]["Code"] == "LimitExceededException":
                            print '    too many requests, sleeping...'                        
                            time.sleep(15)
                        else:
                            print '      ERROR:', e
                            break   
            if token is None:
                break

    def _delete_sqs_queues(self):
        print '\n\nlooking for SQS queues with names starting with one of', self.__describe_prefixes()
        res = self.sqs.list_queues()  
        if 'QueueUrls' not in res:      
            return
        for queueurl in res['QueueUrls']:
            queue_parts = queueurl.split("/")
            queue = queue_parts[4]
            if not self._has_prefix(queue):                
                continue
            print '  found queue', queue
            while True:
                try:
                    print '    deleting queue', queue
                    self.sqs.delete_queue(
                                    QueueUrl=queueurl
                                )
                    break
                except ClientError as e:
                    if e.response["Error"]["Code"] == "LimitExceededException":
                        print '    too many requests, sleeping...'                        
                        time.sleep(15)
                    else:
                        print '      ERROR:', e
                        break    

    def _delete_dynamodb_tables(self):
        print '\n\nlooking for dynamo tables with names starting with one of', self.__describe_prefixes()
        res = self.dynamodb.list_tables()        
        for table in res['TableNames']:
            if not self._has_prefix(table):                
                continue
            print '  found table', table
            while True:
                try:
                    print '    deleting table', table
                    self.dynamodb.delete_table(
                                    TableName=table
                                )
                    break
                except ClientError as e:
                    if e.response["Error"]["Code"] == "LimitExceededException":
                        print '    too many requests, sleeping...'                        
                        time.sleep(15)
                    else:
                        print '      ERROR:', e
                        break               

    @global_resource
    def _delete_buckets(self):

        print '\n\nlooking for buckets with names starting with one of', self.__describe_prefixes()

        res = self.s3.list_buckets()
        while True:
            for bucket in res['Buckets']:
                # print 'considering bucket', bucket['Name']
                if not self._has_prefix(bucket['Name']):
                    # print '  not in prefix list'
                    continue
                print '  found bucket', bucket['Name']
                self._clean_bucket(bucket['Name'])
                try:
                    print '    deleting bucket', bucket['Name']
                    self.s3.delete_bucket(Bucket=bucket['Name'])
                    pass
                except ClientError as e:
                    print '      ERROR', e.message
            next_token = res.get('NextToken', None)
            if next_token is None:
                break
            else:
                res = self.s3.list_buckets(NextToken=next_token)

    def _delete_stacks(self):
        
        print '\n\nlooking for stacks with names starting with one of', self.__describe_prefixes()

        stack_list = []
        res = self.cf.list_stacks()
        while True:
            for stack in res['StackSummaries']:
                # print 'Considering stack', stack['StackName']
                if not self._has_prefix(stack['StackName']):
                    # print '  not in prefixes list'
                    continue
                if stack['StackStatus'] == 'DELETE_COMPLETE' or stack['StackStatus'] == 'DELETE_IN_PROGRESS':
                    # print '  already has deleted status'
                    continue
                stack_list.append(stack)
            next_token = res.get('NextToken', None)
            if next_token is None:
                break
            else:
                res = self.cf.list_stacks(NextToken=next_token)

        for stack in stack_list:
            print '  found stack', stack['StackStatus'], stack['StackId']
            retained_resources = self._clean_stack(stack['StackId'])
            try:
                print '    deleting stack', stack['StackId']
                if stack['StackStatus'] == 'DELETE_FAILED':
                    res = self.cf.delete_stack(StackName=stack['StackId'], RetainResources=retained_resources)
                else:
                    res = self.cf.delete_stack(StackName=stack['StackId'])
            except ClientError as e:
                print '      ERROR', e.message

    def _clean_stack(self, stack_id):
        retained_resources = []
        try:
            print '    getting resources for stack', stack_id
            response = self.cf.describe_stack_resources(StackName=stack_id)
        except ClientError as e:
            print '      ERROR', e.response
            return
        for resource in response['StackResources']:
            resource_id = resource.get('PhysicalResourceId', None)
            if resource_id is not None:
                if resource['ResourceType'] == 'AWS::CloudFormation::Stack':
                    self._clean_stack(resource_id)
                if resource['ResourceType'] == 'AWS::S3::Bucket':
                    self._clean_bucket(resource_id)
                if resource['ResourceType'] == 'AWS::IAM::Role':
                    self._clean_role(resource_id)
            if resource['ResourceStatus'] == 'DELETE_FAILED':
                retained_resources.append(resource['LogicalResourceId'])
        return retained_resources

    def _clean_bucket(self, bucket_id):
        print '    cleaning bucket', bucket_id
        list_res = {}
        try:
            list_res = self.s3.list_object_versions(Bucket=bucket_id, MaxKeys=1000)
        except ClientError as e:
            print e
            if e.response['Error']['Code'] == 'NoSuchBucket':
                return
        while True:

            delete_list = []

            for version in list_res.get('Versions', []):
                print '      deleting object', version['Key'], version['VersionId']
                delete_list.append(
                    {
                        'Key': version['Key'],'VersionId': version['VersionId']
                    })

            for marker in list_res.get('DeleteMarkers', []):
                print '      deleting object', marker['Key'], marker['VersionId'], '(delete marker)'
                delete_list.append(
                    {
                        'Key': marker['Key'],'VersionId': marker['VersionId']
                    })

            if not len(delete_list):
                break

            try:
                delete_res = self.s3.delete_objects(Bucket=bucket_id, Delete={ 'Objects': delete_list, 'Quiet': True })
            except ClientError as e:
                print '        ERROR', e.message
                return

            try:
                list_res = self.s3.list_object_versions(Bucket=bucket_id, MaxKeys=1000)
            except ClientError as e:
                if e.response['Error']['Code'] == 'NoSuchBucket':
                    return

    def _clean_role(self, resource_id):
        
        print '    cleaning role', resource_id

        # delete policies

        try:
            res = self.iam.list_role_policies(RoleName=resource_id)
        except ClientError as e:
            if e.response['Error']['Code'] != 'NoSuchEntity':
                print '      ERROR:', e
            return
        
        for policy_name in res['PolicyNames']:
            print '      deleting policy', policy_name
            try:
                self.iam.delete_role_policy(RoleName=resource_id, PolicyName=policy_name)
            except ClientError as e:
                print '        ERROR:', e

        # detach policies

        try:
            res = self.iam.list_attached_role_policies(RoleName=resource_id)
        except ClientError as e:
            if e.response['Error']['Code'] != 'NoSuchEntity':
                print '      ERROR:', e
            return
        
        for attached_policy in res['AttachedPolicies']:
            print '      detaching policy', attached_policy['PolicyName']
            try:
                self.iam.detach_role_policy(RoleName=resource_id, PolicyArn=attached_policy['PolicyArn'])
            except ClientError as e:
                print '        ERROR:', e


    def _clean_user(self, resource_id):
        
        print '    cleaning user', resource_id

        # delete policies

        try:
            res = self.iam.list_user_policies(UserName=resource_id)
        except ClientError as e:
            if e.response['Error']['Code'] != 'NoSuchEntity':
                print '      ERROR:', e
            return
        
        for policy_name in res['PolicyNames']:
            print '      deleting policy', policy_name
            try:
                self.iam.delete_user_policy(UserName=resource_id, PolicyName=policy_name)
            except ClientError as e:
                print '        ERROR:', e

        # detach policies

        try:
            res = self.iam.list_attached_user_policies(UserName=resource_id)
        except ClientError as e:
            if e.response['Error']['Code'] != 'NoSuchEntity':
                print '      ERROR:', e
            return
        
        for attached_policy in res['AttachedPolicies']:
            print '      detaching policy', attached_policy['PolicyName']
            try:
                self.iam.detach_user_policy(UserName=resource_id, PolicyArn=attached_policy['PolicyArn'])
            except ClientError as e:
                print '        ERROR:', e

        # delete access keys

        try:
            res = self.iam.list_access_keys(UserName=resource_id)
        except ClientError as e:
            if e.response['Error']['Code'] != 'NoSuchEntity':
                print '      ERROR:', e
            return
        
        for access_key_metadata in res['AccessKeyMetadata']:
            print '      deleting access key', access_key_metadata['AccessKeyId']
            try:
                self.iam.delete_access_key(UserName=resource_id, AccessKeyId=access_key_metadata['AccessKeyId'])
            except ClientError as e:
                print '        ERROR:', e

                          
    def _delete_identity_pools(self):

        print '\n\nlooking for cognito identity pools with names starting with one of', self.__describe_prefixes()

        res = self.cognito_identity.list_identity_pools(MaxResults=60)
        while True:
            for pool in res['IdentityPools']:
                # The congitio pools created by CCRM are prefixed with either PlayerAccess or PlayerLogin,
                # followed by the stack name (which is matched against the prefixed passed to this script)

                name = pool['IdentityPoolName']
                for removed in ['PlayerAccess', 'PlayerLogin']:
                    if name.startswith(removed):
                        name = name.replace(removed, '')
                        break;

                if not self._has_prefix(name):
                    continue
                try:
                    print '  deleting identity pool', pool['IdentityPoolName']
                    self.cognito_identity.delete_identity_pool(IdentityPoolId=pool['IdentityPoolId'])
                    pass
                except ClientError as e:
                    print '    ERROR', e.message
            next_token = res.get('NextToken', None)
            if next_token is None:
                break
            else:
                res = self.cognito_identity.list_identity_pools(NextToken=next_token)

    def _delete_user_pools(self):

        print '\n\nlooking for cognito user pools with names starting with one of', self.__describe_prefixes()

        res = self.cognito_idp.list_user_pools(MaxResults=60)
        while True:
            for pool in res['UserPools']:
                # The congitio pools created by CloudGemPlayerAccount are prefixed with PlayerAccess,
                # followed by the stack name (which is matched against the prefixed passed to this script)

                name = pool['Name']
                if name.startswith('PlayerAccess'):
                    name = name.replace('PlayerAccess', '')

                if not self._has_prefix(name):
                    continue
                try:
                    print '  deleting user pool', pool['Name']
                    self.cognito_idp.delete_user_pool(UserPoolId=pool['Id'])
                except ClientError as e:
                    print '    ERROR', e.message
            next_token = res.get('NextToken', None)
            if next_token is None:
                break
            else:
                res = self.cognito_idp.list_user_pools(MaxResults=60, NextToken=next_token)

    @global_resource
    def _delete_roles(self):

        print '\n\nlooking for roles with names or paths starting with one of', self.__describe_prefixes()

        res = self.iam.list_roles()
        while True:

            for role in res['Roles']:
                    
                path = role['Path'][1:] # remove leading /

                if not self._has_prefix(path) and not self._has_prefix(role['RoleName']):
                    continue

                print '  cleaning role', role['RoleName']

                self._clean_role(role['RoleName'])

                try:
                    print '    deleting role', role['RoleName']
                    self.iam.delete_role(RoleName=role['RoleName'])
                except ClientError as e:
                    print '      ERROR:', e

            marker = res.get('Marker', None)
            if marker is None:
                break
            else:
                res = self.iam.list_roles(Marker=marker)

    @global_resource
    def _delete_users(self):

        print '\n\nlooking for users with names or paths starting with one of', self.__describe_prefixes()

        res = self.iam.list_users()
        while True:

            for user in res['Users']:
                    
                path = user['Path'][1:] # remove leading /

                if not self._has_prefix(path) and not self._has_prefix(user['UserName']):
                    continue

                print '  cleaning user', user['UserName']

                self._clean_user(user['UserName'])

                try:
                    print '    deleting user', user['UserName']
                    self.iam.delete_user(UserName=user['UserName'])
                except ClientError as e:
                    print '      ERROR:', e

            marker = res.get('Marker', None)
            if marker is None:
                break
            else:
                res = self.iam.list_users(Marker=marker)

    @global_resource
    def _delete_policies(self):

        print '\n\nlooking for policies with names or paths starting with one of', self.__describe_prefixes()

        res = self.iam.list_policies()
        while True:

            for policy in res['Policies']:
                    
                path = policy['Path'][1:] # remove leading /

                if not self._has_prefix(path) and not self._has_prefix(policy['PolicyName']):
                    continue

                try:
                    print '    deleting policy', policy['Arn']
                    self.iam.delete_policy(PolicyArn=policy['Arn'])
                except ClientError as e:
                    print '      ERROR:', e

            marker = res.get('Marker', None)
            if marker is None:
                break
            else:
                res = self.iam.list_policies(Marker=marker)


    def _delete_log_groups(self):

        print '\n\nlooking for log groups starting with one of', self.__describe_prefixes()

        res = self.logs.describe_log_groups()
        while True:

            for log_group in res['logGroups']:

                name = log_group['logGroupName']
                name = name.replace('/aws/lambda/', '')
                if self._has_prefix(name):

                    try:
                        print '  deleting log group', log_group['logGroupName']
                        self.logs.delete_log_group(logGroupName = log_group['logGroupName'])
                    except ClientError as e:
                        print '      ERROR:', e

            next_token = res.get('nextToken', None)
            if next_token is None:
                break
            else:
                res = self.logs.describe_log_groups(nextToken=next_token)

    def _delete_api_gateway(self):

        print '\n\nlooking for api gateway resources starting with one of', self.__describe_prefixes()

        res = self.apigateway.get_rest_apis()
        while True:

            for rest_api in res['items']:

                name = rest_api['name']
                if self._has_prefix(name):

                    while True:
                        try:
                            print '  deleting rest_api', rest_api['name'], rest_api['id']
                            self.apigateway.delete_rest_api(restApiId = rest_api['id'])
                            break
                        except ClientError as e:
                            if e.response["Error"]["Code"] == "TooManyRequestsException":
                                print '    too many requests, sleeping...'
                                time.sleep(15)
                            else:
                                print '      ERROR:', e
                                break

            position = res.get('position', None)
            if position is None:
                break
            else:
                res = self.apigateway.get_rest_apis(position=position)

    def _delete_lambdas(self):

        print '\n\nlooking for lambda functions starting with one of', self.__describe_prefixes()

        res = self.lambda_client.list_functions()
        while True:

            for info in res['Functions']:

                name = info['FunctionName']
                arn = info['FunctionArn']
                if self._has_prefix(name):

                    while True:
                        try:
                            print '  deleting lambda', name, arn
                            self.lambda_client.delete_function(FunctionName=arn)
                            break
                        except ClientError as e:
                            if e.response["Error"]["Code"] == "TooManyRequestsException":
                                print '    too many requests, sleeping...'
                                time.sleep(15)
                            else:
                                print '      ERROR:', e
                                break

            position = res.get('NextMarker', None)
            if position is None:
                break
            else:
                res = self.lambda_client.list_functions(Marker=position)

    def _delete_iot_policies(self):

        print '\n\nlooking for iot policies with names starting with one of', self.__describe_prefixes()

        res = self.iot_client.list_policies(pageSize=100)
        while True:
            for thisPolicy in res['policies']:
                # Iot policies are created by the WebCommunicator Gem

                name = thisPolicy['policyName']
                if not self._has_prefix(name):
                    continue
                # Clean up old versions of the policy
                version_list = self.iot_client.list_policy_versions(policyName=name)
                for thisVersion in version_list['policyVersions']:
                    if not thisVersion['isDefaultVersion']:
                        try:
                            print '  deleting iot policy {} version {}'.format(name, thisVersion['versionId'])    
                            self.iot_client.delete_policy_version(policyName=name, policyVersionId=thisVersion['versionId'])
                        except ClientError as e:
                            print '    ERROR', e.message
                # Now detach the policy from any principals
                principal_list = self.iot_client.list_policy_principals(policyName=name, pageSize=100)
                next_marker = principal_list.get('nextMarker')
                while True:
                    for thisPrincipal in principal_list['principals']:
                        if ':cert/' in thisPrincipal:
                            # For a cert, the principal is the full arn
                            principal_name = thisPrincipal
                        else:
                            ## Response is in the form of accountId:CognitoId - when we detach we only want cognitoId
                            principal_name = thisPrincipal.split(':',1)[1]
                        print '  Detaching policy {} from principal {}'.format(name, principal_name)
                        try:
                            self.iot_client.detach_principal_policy(policyName=name, principal=principal_name)
                        except ClientError as e:
                            print '    ERROR', e.message

                    if next_marker is None:
                        break
                    principal_list = self.iot_client.list_policy_principals(policyName=name, pageSize=100, marker=next_marker)  
                    next_marker = principal_list.get('nextMarker')
                try:
                    print '  deleting iot policy {}'.format(name)
                    self.iot_client.delete_policy(policyName=name)
                except ClientError as e:
                    print '    ERROR', e.message
            next_marker = res.get('nextMarker', None)
            if next_marker is None:
                break
            else:
                res = self.iot_client.list_policies(pageSize=100, marker=next_marker)

    def _delete_swf_resources(self):

        print '\n\nlooking for SWF resources with names starting with one of', self.__describe_prefixes()

        res = self.swf.list_domains(registrationStatus='REGISTERED', maximumPageSize=100)
        while True:
            for domain in res['domainInfos']:          
                name = domain['name']
                if not self._has_prefix(name):
                    continue    
                try:
                    print '  deleting SWF domain {}'.format(name)
                    self.swf.deprecate_domain(name=name)
                except ClientError as e:
                    print '    ERROR', e.message
            next_marker = res.get('nextPageToken', None)
            if next_marker is None:
                break
            else:
                res = self.swf.list_domains(maximumPageSize=100, nextPageToken=next_marker)
                
if __name__ == '__main__':
    def __get_user_settings_path():
        '''Reads {root}\bootstrap.cfg to determine the name of the game directory.'''

        root_directory_path = os.getcwd()
        while os.path.basename(root_directory_path) != 'dev':
            root_directory_path = os.path.dirname(root_directory_path)

        path = os.path.join(root_directory_path, "bootstrap.cfg")

        if not os.path.exists(path):
            print 'Warning: a bootstrap.cfg file was not found at {}, using "Game" as the project directory name.'.format(path)
            return 'Game'
        else:
            # If we add a section header and change the comment prefix, then
            # bootstrap.cfg looks like an ini file and we can use ConfigParser.
            ini_str = '[default]\n' + open(path, 'r').read()
            ini_str = ini_str.replace('\n--', '\n#')
            ini_fp = StringIO(ini_str)
            config = RawConfigParser()
            config.readfp(ini_fp)

            game_directory_name = config.get('default', 'sys_game_folder')

            platform_mapping = {"win32": "pc", "darwin": "osx_gl"}
            user_settings_path = os.path.join(root_directory_path, 'Cache', game_directory_name, platform_mapping[sys.platform], 'User\AWS\user-settings.json')

            if not os.path.exists(user_settings_path):
                raise HandledError('{} does not exist.'.format(user_settings_path))

            return user_settings_path

    def __get_default_profile(args):
        user_settings_path = __get_user_settings_path()
        profile = 'default'

        if os.path.isfile(user_settings_path):
            with open(user_settings_path, 'r') as file:
                settings = json.load(file)
                profile = settings.get('DefaultProfile', args.profile)

        return profile

    parser = argparse.ArgumentParser()
    parser.add_argument('--prefix', dest='prefixes', nargs='+', default=DEFAULT_PREFIXES, help='Any stacks and buckets with names that start with this value will be deleted.')
    parser.add_argument('--except', dest='exceptions', nargs='+', default=[], help='Do not delete anything starting with these prefixes (useful for cleaning up old results while a test is in progress).')
    parser.add_argument('--profile', default='default', help='The AWS profile to use. Defaults to the default AWS profile.')
    parser.add_argument('--aws-access-key', required=False, help='The AWS access key to use.')
    parser.add_argument('--aws-secret-key', required=False, help='The AWS secret key to use.')
    parser.add_argument('--region', default=None, help='The AWS region to use. Defaults to {}.'.format(DEFAULT_REGION))
    parser.add_argument('--delete-global-resources', action='store_true', default=False, help='If --region specified, use to delete global resources such as IAM roles and S3 buckets. Ignored if --region is unspecified.')

    args = parser.parse_args()

    prefixes = []
    for prefix in args.prefixes:
        prefixes.append(prefix.lower())

    exceptions = []
    for exception in args.exceptions:
        exceptions.append(exception.lower())
        
    use_region = args.region if args.region else DEFAULT_REGION

    if args.aws_access_key and args.aws_secret_key:
        session = boto3.Session(aws_access_key_id=args.aws_access_key, aws_secret_access_key=args.aws_secret_key, region_name=use_region)
    elif os.environ.get('NO_TEST_PROFILE', None):
        session = boto3.Session(region_name=use_region)
    else:
        args.profile = __get_default_profile(args)
        session = boto3.Session(profile_name=args.profile, region_name=use_region)
        
    delete_global_resources = True
    if args.region and not args.delete_global_resources:
        print("WARNING: Specifying a --region skips deletion of global resources like IAM roles and S3 buckets. Add --delete-global-resources or do not specify a region to include these resources in deletion.")
        delete_global_resources = False
    elif args.delete_global_resources and not args.region:
        print("WARNING: --delete-global-resources is ignored without specifying a --region. (Global resources are always included for deletion if --region is left unspecified.)")
    elif not args.region:
        print("WARNING: No --region was specified.  The cleanup will default to the region '{}' and delete any matching global resources.".format(DEFAULT_REGION))
        
    cleaner = Cleaner(session, delete_global_resources)
    cleaner.cleanup(prefixes, exceptions)
