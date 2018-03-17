#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the 'License'). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# $Revision$
'''Creates a zip file that can be used to re-create project and deployment stacks 
without any external tools (such as lmbr_aws). This is used to capture stacks that
can be used as the starting point for project update testing. It isn't intended to
be a comprehensive backup and restore solution.
'''

from __future__ import print_function

import argparse
import contextlib
import copy
import json
import os
import shutil
import stat
import sys
import tempfile
import time
import urllib
import zipfile

import boto3

import resource_manager.aws

REPLACE_ME = '-REPLACE-ME-'

REPLACED_PARAMETER_KEYS = [
    'ConfigurationBucket', 
    'DeploymentStack', 
    'DeploymentStackArn', 
    'PlayerAccessTokenExchange',
    'ProjectResourceHandler', 
    'ProjectStack', 
    'ProjectStackId', 
    'ResourceHandler'
]

SNAPSHOT_BUCKET_DIRECTORY_NAME = 'buckets'
SNAPSHOT_COPIED_GEMS_DIRECTORY_PLACEHOLDER = '-COPIED-GEMS-'
SNAPSHOT_DEPLOYMENT_ACCESS_STACK_DIRECTORY_NAME = 'deployment-access-stack'
SNAPSHOT_DEPLOYMENT_STACK_DIRECTORY_NAME = 'deployment-stack'
SNAPSHOT_GEMS_DIRECTORY_NAME = 'gems'
SNAPSHOT_PROJECT_DIRECTORY_NAME = 'project-directory'
SNAPSHOT_PROJECT_GEM_DIRECTORY_PLACEHOLDER = '-PROJECT-GEM-'
SNAPSHOT_ROOT_GEMS_DIRECTORY_PLACEHOLDER = '-ROOT-GEMS-'
SNAPSHOT_PROJECT_STACK_DIRECTORY_NAME = 'project-stack'
SNAPSHOT_STACK_PARAMETERS_FILE_NAME = 'stack-parameters.json'
SNAPSHOT_SUBSTACK_DIRECTORY_NAME = 'stacks'

session = None

##
## Common
#####################################################################################################################################
## Create
##

def get_create_subparser(subparsers):

    create_parser = subparsers.add_parser('create', help='Create a snapshot of a project.')
    
    create_parser.add_argument(
        '--project-directory', '-p', 
        metavar='PATH', 
        required=True, 
        dest='project_directory_path', 
        help='The path to the project directory (or the name of the project directory in the current directory).')

    create_parser.add_argument(
        '--snapshot-file', '-s', 
        metavar='PATH', 
        required=True, 
        dest='snapshot_file_path', 
        help='The path to the zip file where the snapshot will be written.')

    create_parser.add_argument(
        '--profile', 
        required=False, 
        default='default', 
        help='Identifies the AWS profile to use. The default is "default".')

    create_parser.add_argument(
        '--gems', '-g', 
        metavar='PATH', 
        nargs='+', 
        required=False, 
        default=[], 
        dest='copied_gem_names', 
        help='The names of gems that will be included in the snapshot. This should NOT include any gem that ships with Lumberyard.')

    create_parser.add_argument(
        '--root-directory',
        metavar='PATH',
        required=False,
        dest='root_directory_path',
        help='The Lumberyard root directory. Defaults to the parent of the project directory.')

    create_parser.set_defaults(handler=create_snapshot)

    return create_parser


def create_snapshot(profile, project_directory_path, snapshot_file_path, copied_gem_names, root_directory_path):

    if root_directory_path is None:
        root_directory_path = os.path.abspath(os.path.join(project_directory_path, '..'))

    snapshot_directory_path = tempfile.mkdtemp(prefix='project_snapshot_')
        
    project_directory_path = os.path.abspath(project_directory_path)
    snapshot_directory_path = os.path.abspath(snapshot_directory_path)

    copy_project_directory(project_directory_path, snapshot_directory_path)
    copy_gem_directories(project_directory_path, snapshot_directory_path, root_directory_path, copied_gem_names)

    project_stack_name, region_name = get_project_stack_and_region_names(project_directory_path)

    if project_stack_name:
        global session
        if os.environ.get('NO_TEST_PROFILE'):
            session = boto3.Session(region_name = region_name)
        else:
            session = boto3.Session(region_name = region_name, profile_name = profile)
        deployment_stack_arns = download_project_stack(project_stack_name, snapshot_directory_path)

        for deployment_name, stack_arns in deployment_stack_arns.iteritems():
            download_deployment_stack(deployment_name, stack_arns['DeploymentStackArn'], snapshot_directory_path)
            download_deployment_access_stack(deployment_name, stack_arns['DeploymentAccessStackArn'], snapshot_directory_path)

    zip_directory(snapshot_directory_path, snapshot_file_path)

    rmtree(snapshot_directory_path)


def copy_project_directory(project_directory_path, snapshot_directory_path):
    
    snapshot_project_directory_path = os.path.join(snapshot_directory_path, SNAPSHOT_PROJECT_DIRECTORY_NAME)
    os.makedirs(snapshot_project_directory_path)

    # copy game.cfg
    shutil.copyfile(os.path.join(project_directory_path, 'game.cfg'), os.path.join(snapshot_project_directory_path, 'game.cfg'))

    # copy gems.json
    snapshot_gems_file_path = os.path.join(snapshot_project_directory_path, 'gems.json')
    project_gems_file_path = os.path.join(project_directory_path, 'gems.json')
    shutil.copyfile(project_gems_file_path, snapshot_gems_file_path)

    # copy AWS directory
    project_aws_directory_path = os.path.join(project_directory_path, 'AWS')
    snapshot_aws_directory_path = os.path.join(snapshot_project_directory_path, 'AWS')
    print('Copying {} to {}.'.format(project_aws_directory_path, snapshot_aws_directory_path))
    shutil.copytree(project_aws_directory_path, snapshot_aws_directory_path)

    # copy Gem directory
    project_gem_directory_path = os.path.join(project_directory_path, 'Gem')
    snapshot_gem_directory_path = os.path.join(snapshot_project_directory_path, 'Gem')
    print('Copying {} to {}.'.format(project_gem_directory_path, snapshot_gem_directory_path))
    shutil.copytree(project_gem_directory_path, snapshot_gem_directory_path)

    # remove ProjectStackId from local-project-settings.json copy
    with json_file_content(os.path.join(snapshot_aws_directory_path, 'local-project-settings.json')) as local_project_settings:
        local_project_settings.pop('ProjectStackId', None)


def copy_gem_directories(project_directory_path, snapshot_directory_path, root_directory_path, copied_gem_names):
    
    project_gem_path = os.path.basename(project_directory_path) + '/Gem'

    with json_file_content(os.path.join(snapshot_directory_path, SNAPSHOT_PROJECT_DIRECTORY_NAME, 'gems.json')) as gems_content:

        for gem in gems_content['Gems']:

            if gem.get('_comment') in copied_gem_names:
                gem['Path'] = copy_gem(gem, root_directory_path, snapshot_directory_path)

            elif gem.get('Path') == project_gem_path:
                gem['Path'] = SNAPSHOT_PROJECT_GEM_DIRECTORY_PLACEHOLDER

            elif gem.get('Path', '').startswith('Gems'):
                gem['Path'] = SNAPSHOT_ROOT_GEMS_DIRECTORY_PLACEHOLDER + gem['Path'][4:] # Gems/... -> PLACEHOLDER/...

            else:
                raise RuntimeError('Unsupported gem configuration: {}'.format(gem))


def copy_gem(gem, root_directory_path, snapshot_directory_path):

    gem_name = gem['_comment'] # vs opening the gem.json and reading it....

    dst_directory_path = os.path.join(snapshot_directory_path, SNAPSHOT_GEMS_DIRECTORY_NAME, gem_name)
    src_directory_path = os.path.join(root_directory_path, gem['Path'])

    print('Copying {} to {}.'.format(src_directory_path, dst_directory_path))

    shutil.copytree(src_directory_path, dst_directory_path)

    return SNAPSHOT_COPIED_GEMS_DIRECTORY_PLACEHOLDER + '/' + gem_name
    

def get_project_stack_and_region_names(project_directory_path):

    local_project_settings = get_json_file_content(os.path.join(project_directory_path, 'AWS', 'local-project-settings.json'))
    stack_arn = local_project_settings.get('ProjectStackId')

    if stack_arn:
        stack_name = get_stack_name_from_stack_arn(stack_arn)
        region_name = get_region_name_from_stack_arn(stack_arn)
    else:
        stack_name = None
        region_name = None

    return (stack_name, region_name)


def download_project_stack(project_stack_name, snapshot_directory_path):
    
    snapshot_project_directory_path = os.path.join(snapshot_directory_path, SNAPSHOT_PROJECT_STACK_DIRECTORY_NAME)

    download_stack(project_stack_name, snapshot_project_directory_path, ignore=["Logs"])
    
    # Get all deployment stack arns from the project settings and save without 
    # stack arns to prevent leading account numbers, etc.
    with json_file_content(os.path.join(snapshot_project_directory_path, SNAPSHOT_BUCKET_DIRECTORY_NAME, 'Configuration', 'project-settings.json')) as project_settings:

        deployment_stack_arns = {}
        for deployment_name, deployment_settings in project_settings.get('deployment', {}).iteritems():
            if deployment_name == '*':
                continue
            deployment_stack_arn = deployment_settings['DeploymentStackId']
            deployment_access_stack_arn = deployment_settings['DeploymentAccessStackId']
            deployment_stack_arns[deployment_name] = {
                'DeploymentStackArn': deployment_stack_arn,
                'DeploymentAccessStackArn': deployment_access_stack_arn
            }
            del deployment_settings['DeploymentStackId']
            del deployment_settings['DeploymentAccessStackId']

    return deployment_stack_arns


def download_deployment_stack(deployment_name, desployment_stack_arn, snapshot_directory_path):
    download_stack(desployment_stack_arn, os.path.join(snapshot_directory_path, SNAPSHOT_DEPLOYMENT_STACK_DIRECTORY_NAME, deployment_name))


def download_deployment_access_stack(deployment_name, desployment_access_stack_arn, snapshot_directory_path):
    download_stack(desployment_access_stack_arn, os.path.join(snapshot_directory_path, SNAPSHOT_DEPLOYMENT_ACCESS_STACK_DIRECTORY_NAME, deployment_name))


def download_stack(stack_name, snapshot_directory_path, ignore=[], save_parameters = True):
    
    print('Downloading stack {} content to {}.'.format(stack_name, snapshot_directory_path))

    if not os.path.exists(snapshot_directory_path):
        os.makedirs(snapshot_directory_path)

    cf = get_cloud_formation_client()

    # filter and write project-parameters.json
    if save_parameters:
        res = cf.describe_stacks(StackName = stack_name)
        parameters = res['Stacks'][0]['Parameters']
        set_parameter_values(parameters, **{ key: REPLACE_ME for key in REPLACED_PARAMETER_KEYS })
        with open(os.path.join(snapshot_directory_path, SNAPSHOT_STACK_PARAMETERS_FILE_NAME), 'w') as file:
            json.dump(parameters, file, indent=4, sort_keys=True)

    # write resource content
    res = cf.describe_stack_resources(StackName = stack_name)
    for resource_description in res['StackResources']:
        if resource_description['LogicalResourceId'] in ignore:
            continue
        handler = DOWNLOAD_RESOURCE_TYPE_HANDLER.get(resource_description['ResourceType'])
        if handler is not None:
            handler(resource_description, snapshot_directory_path)


def download_bucket_content(resource_description, snapshot_directory_path):
    
    logical_resource_id = resource_description['LogicalResourceId']
    physical_resource_id = resource_description['PhysicalResourceId']
    content_directory_path = os.path.join(snapshot_directory_path, SNAPSHOT_BUCKET_DIRECTORY_NAME, logical_resource_id)

    print('Downloading {} bucket objects to {}.'.format(logical_resource_id, content_directory_path))
    
    if not os.path.exists(content_directory_path):
        os.makedirs(content_directory_path)

    s3 = get_s3_client()

    paginator = s3.get_paginator('list_objects')
    for res in paginator.paginate(Bucket=physical_resource_id):
        for content in res.get('Contents'):

            dst_key = content['Key'].replace('/', os.sep)
            
            if dst_key.startswith(os.sep):
                dst_key = dst_key[1:]

            dst_file_path = os.path.join(content_directory_path, dst_key)

            print('Downloading {} bucket object {} to {}.'.format(physical_resource_id, content['Key'], dst_file_path))

            dst_directory_path =os.path.dirname(dst_file_path)
            if not os.path.exists(dst_directory_path):
                os.makedirs(dst_directory_path)

            s3.download_file(physical_resource_id, content['Key'], dst_file_path)


def download_stack_content(resource_description, snapshot_directory_path):

    logical_resource_id = resource_description['LogicalResourceId']
    physical_resource_id = resource_description['PhysicalResourceId']
    content_directory_path = os.path.join(snapshot_directory_path, SNAPSHOT_SUBSTACK_DIRECTORY_NAME, logical_resource_id)

    download_stack(physical_resource_id, content_directory_path, save_parameters = False)


DOWNLOAD_RESOURCE_TYPE_HANDLER = {
    'AWS::S3::Bucket': download_bucket_content,
    'AWS::CloudFormation::Stack': download_stack_content
}

##
## Create 
#####################################################################################################################################
## Restore
##

def get_restore_subparser(subparsers):

    restore_parser = subparsers.add_parser('restore', help='Restore a project from a snapshot.')

    restore_parser.add_argument(
        '--project-directory', '-p', 
        metavar='PATH', 
        required=True, 
        dest='project_directory_path', 
        help='The path to the project directory (or the name of the project directory in the current directory).')

    restore_parser.add_argument(
        '--snapshot-file', '-s', 
        metavar='PATH', 
        required=True, 
        dest='snapshot_file_path', 
        help='The path to the zip file that contains the snapshot.')

    restore_parser.add_argument(
        '--profile', 
        required=False, 
        default='default', 
        help='Identifies the AWS profile to use. The default is "default".')

    restore_parser.add_argument(
        '--region', 
        required=False, 
        default='us-east-1', 
        help='The AWS region to use when creating resources.')

    restore_parser.add_argument(
        '--stack-name', 
        required=False, 
        help='The name of the project stack and the pefix for the deployment stack names. Required if the snapshot includes a project stack.')

    restore_parser.add_argument(
        '--root-directory',
        metavar='PATH',
        required=False,
        dest='root_directory_path',
        help='The Lumberyard root directory. Defaults to the parent of the project directory.')

    restore_parser.set_defaults(handler=restore_snapshot)


def restore_snapshot(region, profile, stack_name, project_directory_path, snapshot_file_path, root_directory_path):

    if root_directory_path is None:
        root_directory_path = os.path.dirname(os.path.abspath(project_directory_path))

    snapshot_directory_path = tempfile.mkdtemp(prefix='project_snapshot_')
    unzip_directory(snapshot_file_path, snapshot_directory_path)

    project_directory_path = os.path.abspath(project_directory_path)
    snapshot_directory_path = os.path.abspath(snapshot_directory_path)

    restore_project_directory(project_directory_path, snapshot_directory_path)
    restore_gems(project_directory_path, snapshot_directory_path, root_directory_path)

    if os.path.exists(os.path.join(snapshot_directory_path, SNAPSHOT_PROJECT_STACK_DIRECTORY_NAME)):

        if not stack_name:
            raise RuntimeError('The snapshot contains a project stack but no --stack-name argument was provided.')

        global session
        if os.environ.get('NO_TEST_PROFILE'):
            session = boto3.Session(region_name = region)
        else:
            session = boto3.Session(region_name = region, profile_name = profile)

        project_stack_arn = upload_project_stack(stack_name, project_directory_path, snapshot_directory_path)

        deployment_stacks_directory_path = os.path.join(snapshot_directory_path, SNAPSHOT_DEPLOYMENT_STACK_DIRECTORY_NAME)
        for deployment_name in os.listdir(deployment_stacks_directory_path):
            deployment_stack_arn = upload_deployment_stack(deployment_name, project_stack_arn, snapshot_directory_path)
            deployment_access_stack_arn = upload_deployment_access_stack(deployment_name, project_stack_arn, snapshot_directory_path, deployment_stack_arn)
            upload_deployment_configuration(project_stack_arn, deployment_name, deployment_stack_arn, deployment_access_stack_arn)

    rmtree(snapshot_directory_path)


def restore_project_directory(project_directory_path, snapshot_directory_path):
    rmtree(project_directory_path)
    snapshot_project_directory_path = os.path.join(snapshot_directory_path, SNAPSHOT_PROJECT_DIRECTORY_NAME)
    print('Copying {} to {}.'.format(snapshot_project_directory_path, project_directory_path))
    shutil.copytree(snapshot_project_directory_path, project_directory_path)


def restore_gems(project_directory_path, snapshot_directory_path, root_directory_path):

    # copy any of the gems archived in the snapshot to the project directory=

    restored_gems_directory_path = os.path.abspath(os.path.join(project_directory_path, 'RestoredGems'))
    snapshot_gems_directory_path = os.path.join(snapshot_directory_path, SNAPSHOT_GEMS_DIRECTORY_NAME)

    for gem_name in os.listdir(snapshot_gems_directory_path):

        restored_gem_directory_path = os.path.join(restored_gems_directory_path, gem_name)
        snapshot_gem_directory_path = os.path.join(snapshot_gems_directory_path, gem_name)

        print('Copying {} to {}.'.format(snapshot_gem_directory_path, restored_gem_directory_path))
        shutil.copytree(snapshot_gem_directory_path, restored_gem_directory_path)

    # fix up the gems.json file to point to the right locations

    project_gem_directory_path = os.path.abspath(os.path.join(project_directory_path, 'Gem'))
    root_gems_directory_path = os.path.abspath(os.path.join(root_directory_path, 'Gems'))

    with json_file_content(os.path.join(project_directory_path, 'gems.json')) as content:
        for gem in content['Gems']:

            path = gem['Path']
            path = path.replace('/', os.sep)
            path = path.replace(SNAPSHOT_PROJECT_GEM_DIRECTORY_PLACEHOLDER, project_gem_directory_path)
            path = path.replace(SNAPSHOT_COPIED_GEMS_DIRECTORY_PLACEHOLDER, restored_gems_directory_path)
            path = path.replace(SNAPSHOT_ROOT_GEMS_DIRECTORY_PLACEHOLDER, root_gems_directory_path)
            gem['Path'] = path

            # Update project's gem version to match available version. Applies only to gems in 
            # in the root directory since they are not included in the snapshot.
            if path.startswith(root_gems_directory_path):

                project_gem_version = gem['Version']

                gem_file_path = os.path.join(path, 'gem.json')
                if not os.path.exists(gem_file_path):
                    raise RuntimeError('Missing gem configuration file: {}'.format(gem_file_path))

                gem_config = get_json_file_content(gem_file_path)
                actual_gem_version = gem_config['Version']

                if actual_gem_version != project_gem_version:
                    print('Updating gem {} version from {} to {}.'.format(gem_config['Name'], project_gem_version, actual_gem_version))
                    gem['Version'] = actual_gem_version


def upload_project_stack(stack_name, project_directory_path, snapshot_directory_path):

    snapshot_project_stack_directory_path = os.path.join(snapshot_directory_path, SNAPSHOT_PROJECT_STACK_DIRECTORY_NAME)

    cf = get_cloud_formation_client()

    res = cf.create_stack(
        StackName = stack_name,
        TemplateBody = BOOTSTRAP_TEMPLATE,
        Capabilities = ['CAPABILITY_IAM'],
        DisableRollback = True
    )
    
    project_stack_arn = res['StackId']

    print('Waiting for create of stack {}.'.format(get_stack_name_from_stack_arn(project_stack_arn)))
    cf.get_waiter('stack_create_complete').wait(StackName = project_stack_arn)

    configuration_bucket_name = get_stack_resource_physical_id(project_stack_arn, 'Configuration')
    
    upload_bucket_content(
        configuration_bucket_name, 
        os.path.join(snapshot_project_stack_directory_path, SNAPSHOT_BUCKET_DIRECTORY_NAME, 'Configuration'))

    parameters = get_json_file_content(os.path.join(snapshot_project_stack_directory_path, SNAPSHOT_STACK_PARAMETERS_FILE_NAME))

    template_object_name = get_parameter_value(parameters, 'ConfigurationKey') + '/project-template.json'

    res = cf.update_stack(
        StackName = stack_name,
        TemplateURL = make_s3_url(configuration_bucket_name, template_object_name),
        Capabilities = ['CAPABILITY_IAM'],
        Parameters = parameters
    )

    print('Waiting for update of stack {}.'.format(get_stack_name_from_stack_arn(project_stack_arn)))
    cf.get_waiter('stack_update_complete').wait(StackName = project_stack_arn)

    upload_stack_resource_content(project_stack_arn, snapshot_project_stack_directory_path, ignore = ['Configuration'])

    with json_file_content(os.path.join(project_directory_path, 'AWS', 'local-project-settings.json')) as local_project_settings:
        local_project_settings['ProjectStackId'] = project_stack_arn

    return project_stack_arn


def upload_bucket_content_handler(resource_description, snapshot_directory_path):

    bucket_name = resource_description['LogicalResourceId']
    
    bucket_content_dirctory_path = os.path.join(snapshot_directory_path, SNAPSHOT_BUCKET_DIRECTORY_NAME, bucket_name)
    
    if not os.path.exists(bucket_content_dirctory_path):
        return

    upload_bucket_content(resource_description['PhysicalResourceId'], bucket_content_dirctory_path)
        

def upload_bucket_content(bucket_name, bucket_content_dirctory_path):
    s3 = get_s3_client()
    for root, directory_names, file_names in os.walk(bucket_content_dirctory_path):
        for file_name in file_names:
            file_path = os.path.join(root, file_name)
            object_name = file_path.replace(bucket_content_dirctory_path + os.sep, '').replace(os.sep, '/')
            print('Uploading {} to bucket {} object {}.'.format(file_path, bucket_name, object_name))
            s3.upload_file(file_path, bucket_name, object_name)


def upload_stack_content_handler(resource_description, snapshot_directory_path):

    logical_resource_id = resource_description['LogicalResourceId']
    physical_resource_id = resource_description['PhysicalResourceId']
    content_directory_path = os.path.join(snapshot_directory_path, SNAPSHOT_SUBSTACK_DIRECTORY_NAME, logical_resource_id)

    upload_stack_resource_content(physical_resource_id, content_directory_path)


UPLOAD_RESOURCE_TYPE_HANDLER = {
    'AWS::S3::Bucket': upload_bucket_content_handler,
    'AWS::CloudFormation::Stack': upload_stack_content_handler
}


def upload_stack_resource_content(stack_arn, snapshot_directory_path, ignore = []):
    cf = get_cloud_formation_client()
    res = cf.describe_stack_resources(StackName = stack_arn)
    for resource_description in res['StackResources']:
        if resource_description['LogicalResourceId'] in ignore:
            continue
        handler = UPLOAD_RESOURCE_TYPE_HANDLER.get(resource_description['ResourceType'])
        if handler is not None:
            handler(resource_description, snapshot_directory_path)


def upload_deployment_stack(deployment_name, project_stack_arn, snapshot_directory_path):
    
    deployment_stack_name = get_stack_name_from_stack_arn(project_stack_arn) + '-' + deployment_name
    deployment_stack_directory_path = os.path.join(snapshot_directory_path, SNAPSHOT_DEPLOYMENT_STACK_DIRECTORY_NAME, deployment_name)

    configuration_bucket_name = get_stack_resource_physical_id(project_stack_arn, 'Configuration')

    parameters = get_json_file_content(os.path.join(deployment_stack_directory_path, SNAPSHOT_STACK_PARAMETERS_FILE_NAME))

    set_parameter_values(
        parameters,
        ProjectStackId = project_stack_arn,
        ConfigurationBucket = configuration_bucket_name,
        ProjectResourceHandler = get_stack_lambda_arn(project_stack_arn, 'ProjectResourceHandler'))

    template_object_name = get_parameter_value(parameters, 'ConfigurationKey') + '/deployment-template.json'

    cf = get_cloud_formation_client()
    res = cf.create_stack(
        StackName = deployment_stack_name,
        TemplateURL = make_s3_url(configuration_bucket_name, template_object_name),
        Capabilities = ['CAPABILITY_IAM'],
        DisableRollback = True,
        Parameters = parameters
    )

    deployment_stack_arn = res['StackId']

    print('Waiting for create of stack {}'.format(get_stack_name_from_stack_arn(deployment_stack_arn)))
    cf.get_waiter('stack_create_complete').wait(StackName = deployment_stack_arn)

    upload_stack_resource_content(deployment_stack_arn, deployment_stack_directory_path)

    return deployment_stack_arn


def upload_deployment_access_stack(deployment_name, project_stack_arn, snapshot_directory_path, deployment_stack_arn):
    
    deployment_access_stack_name = get_stack_name_from_stack_arn(project_stack_arn) + '-' + deployment_name + '-Access'
    deployment_access_stack_directory_path = os.path.join(snapshot_directory_path, SNAPSHOT_DEPLOYMENT_ACCESS_STACK_DIRECTORY_NAME, deployment_name)

    configuration_bucket_name = get_stack_resource_physical_id(project_stack_arn, 'Configuration')

    parameters = get_json_file_content(os.path.join(deployment_access_stack_directory_path, SNAPSHOT_STACK_PARAMETERS_FILE_NAME))
    set_parameter_values(parameters, 
        ProjectStack = get_stack_name_from_stack_arn(project_stack_arn),
        ConfigurationBucket = configuration_bucket_name,
        ProjectResourceHandler = get_stack_lambda_arn(project_stack_arn, 'ProjectResourceHandler'),
        PlayerAccessTokenExchange = get_stack_lambda_arn(project_stack_arn, 'PlayerAccessTokenExchange'),
        DeploymentStack = get_stack_name_from_stack_arn(project_stack_arn),
        DeploymentStackArn = deployment_stack_arn)

    template_object_name = get_parameter_value(parameters, 'ConfigurationKey') + '/deployment-access-template.json'

    cf = get_cloud_formation_client()
    res = cf.create_stack(
        StackName = deployment_access_stack_name,
        TemplateURL = make_s3_url(configuration_bucket_name, template_object_name),
        Capabilities = ['CAPABILITY_IAM'],
        DisableRollback = True,
        Parameters = parameters
    )

    deployment_access_stack_arn = res['StackId']

    print('Waiting for create of stack {}.'.format(get_stack_name_from_stack_arn(deployment_access_stack_arn)))
    cf.get_waiter('stack_create_complete').wait(StackName = deployment_access_stack_arn)

    upload_stack_resource_content(deployment_access_stack_arn, deployment_access_stack_directory_path)

    return deployment_access_stack_arn


def upload_deployment_configuration(project_stack_arn, deployment_name, deployment_stack_arn, deployment_access_stack_arn):

    print('Uploading deployment {} configuration.'.format(deployment_name))

    configuration_bucket_name = get_stack_resource_physical_id(project_stack_arn, 'Configuration')

    with json_object_content(Bucket = configuration_bucket_name, Key = 'project-settings.json') as project_settings:
        deployment_settings = project_settings.setdefault('deployment', {}).setdefault(deployment_name, {})
        deployment_settings['DeploymentStackId'] = deployment_stack_arn
        deployment_settings['DeploymentAccessStackId'] = deployment_access_stack_arn


##
## Restore 
#####################################################################################################################################
## Common
##

def get_stack_name_from_stack_arn(arn):
    # Stack ARN format: arn:aws:cloudformation:{region}:{account}:stack/{name}/{guid}
    return arn.split('/')[1]


def get_region_name_from_stack_arn(arn):
    # Stack ARN format: arn:aws:cloudformation:{region}:{account}:stack/{name}/{guid}
    return arn.split(':')[3]


def get_account_from_stack_arn(arn):
    # Stack ARN format: arn:aws:cloudformation:{region}:{account}:stack/{name}/{guid}
    return arn.split(':')[4]


__physical_resource_id_cache = {}
def get_stack_resource_physical_id(stack_arn, resource_name):
    stack_cache = __physical_resource_id_cache.setdefault(stack_arn, {})
    if resource_name not in stack_cache:
        cf = get_cloud_formation_client()
        res = cf.describe_stack_resource(StackName = stack_arn, LogicalResourceId = resource_name)
        stack_cache[resource_name] = res['StackResourceDetail']['PhysicalResourceId']
    return stack_cache[resource_name]


def get_stack_lambda_arn(stack_arn, resource_name):
    physical_resource_id = get_stack_resource_physical_id(stack_arn, resource_name)
    return 'arn:aws:lambda:{region}:{account}:function:{function}'.format(
        region = get_region_name_from_stack_arn(stack_arn),
        account = get_account_from_stack_arn(stack_arn),
        function = physical_resource_id)
    

def make_s3_url(bucket, key):
    return 'https://{bucket}.s3.amazonaws.com/{key}'.format(bucket=bucket, key=key)


def get_stack_arn(stack_name):
    try:
        cf = get_cloud_formation_client()
        res = cf.describe_stacks(StackName = stack_name)
        return res['Stacks'][0]['StackId']
    except:
        return None


def rmtree(path):

    # fast ssd introduce a race condition when removing directories where the 
    # directory is empty but the os still reports it has contents. A slight
    # delay before retrying works around the problem.
    def retry_rmdir(action, name, exc):
        if action == os.rmdir: 
            time.sleep(0.1)
            os.rmdir(name)
        else:
            raise RuntimeError('Error when removing {}: {}'.format(name, exc))

    if os.path.exists(path):
        print('Deleting {}.'.format(path))
        shutil.rmtree(path, onerror=retry_rmdir)

    time.sleep(0.1)


def set_parameter_values(parameters, **kwargs):
    for parameter in parameters:
        new_value = kwargs.get(parameter['ParameterKey'], None)
        if new_value is not None:
            parameter['ParameterValue'] = new_value


def get_parameter_value(parameters, key):
    for parameter in parameters:
        if parameter['ParameterKey'] == key:
            return parameter['ParameterValue']
    return None


@contextlib.contextmanager
def json_file_content(file_path):

    with open(file_path, 'r') as file:
        content = json.load(file)

    yield content

    with open(file_path, 'w') as file:
        json.dump(content, file, indent=4, sort_keys=True)


@contextlib.contextmanager
def json_object_content(**kvargs):

    s3 = get_s3_client()
    res = s3.get_object(**kvargs)
    content = json.loads(res['Body'].read())

    yield content

    s3.put_object(Body = json.dumps(content, indent=4, sort_keys=True), **kvargs)


def get_json_file_content(file_path):

    with open(file_path, 'r') as file:
        return json.load(file)
    

def zip_directory(directory_path, file_path):

    print('Zipping {} to {}.'.format(directory_path, file_path))

    if(file_path.endswith('.zip')):
        file_path = file_path[:-4]

    shutil.make_archive(file_path, 'zip', directory_path)


def unzip_directory(file_path, directory_path):

    if not file_path.endswith('.zip'):
        file_path += '.zip'

    print('Unzipping {} to {}.'.format(file_path, directory_path))

    with zipfile.ZipFile(file_path, 'r') as zip:
        zip.extractall(directory_path)


def get_cloud_formation_client():
    return resource_manager.aws.CloudFormationClientWrapper(session.client('cloudformation'), verbose = False)

def get_s3_client():
    return resource_manager.aws.ClientWrapper(session.client('s3'), verbose = False)
            
##
## Common
#####################################################################################################################################
## Templates
##

BOOTSTRAP_TEMPLATE = '''{
    "AWSTemplateFormatVersion": "2010-09-09",

    "Parameters": {

        "CloudCanvasStack": {
            "Type": "String",
            "Description": "Identifies this stack as a Lumberyard Cloud Canvas managed stack.",
            "Default": "Project"
        }

    },

    "Resources": {

        "Configuration": {
            "Type": "AWS::S3::Bucket",
            "Properties": {
                "VersioningConfiguration": {
                    "Status": "Enabled"
                },
                "LifecycleConfiguration": {
                    "Rules": [
                        {
                            "Id": "DeleteOldVersions",
                            "NoncurrentVersionExpirationInDays": "2",
                            "Status": "Enabled"
                        },
                        {
                            "Id": "DeleteUploads",
                            "Prefix": "uploads",
                            "ExpirationInDays": 2,
                            "Status": "Enabled"
                        }
                    ]
                }
            }
        }

    }

}'''

##
## Templates
#####################################################################################################################################
## Main
##

def main():

    parser = argparse.ArgumentParser(
        prog = 'project_snapshot',
        description='Captures the data needed to recreate project and deployment stacks without using lmbr_aws as needed to support version upgrade testing.'
    )

    subparsers = parser.add_subparsers()
    get_create_subparser(subparsers)
    get_restore_subparser(subparsers)

    args = parser.parse_args()

    handler = args.handler
    del args.handler

    handler(**vars(args))

    return 0

if __name__ == "__main__":
    sys.exit(main())
