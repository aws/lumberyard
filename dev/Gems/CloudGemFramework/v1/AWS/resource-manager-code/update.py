#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

import os
import swagger_processor
import resource_manager_common.constant as constant
import resource_manager.util
import re
import random
import string
import sys
import cStringIO
import json
import base64

from cgf_utils.version_utils import Version

from resource_manager.uploader import Uploader
from resource_manager.errors import HandledError
from botocore.client import Config
from botocore.exceptions import ClientError

AWS_S3_BUCKET_NAME = 'CloudGemPortal'
BUCKET_ROOT_DIRECTORY_NAME = 'www'
BOOTSTRAP_VARIABLE_NAME = "var bootstrap ="
BOOTSTRAP_REGEX_PATTERN = '<script>\s*var bootstrap\s*=\s*([\s\S}]*?)<\/script>'


def before_resource_group_updated(hook, resource_group_uploader, **kwargs):
    swagger_path = os.path.join(resource_group_uploader.resource_group.directory_path, 'swagger.json')
    if os.path.isfile(swagger_path):
        resource_group_uploader.context.view.processing_swagger(swagger_path)
        result = swagger_processor.process_swagger_path(hook.context, swagger_path)
        resource_group_uploader.upload_content('swagger.json', result, 'processed swagger')


def before_project_updated(hook, project_uploader, **kwargs):
    swagger_path = os.path.join(project_uploader.context.config.framework_aws_directory_path, 'swagger.json')
    if os.path.isfile(swagger_path):
        project_uploader.context.view.processing_swagger(swagger_path)
        result = swagger_processor.process_swagger_path(hook.context, swagger_path)
        project_uploader.upload_content('swagger.json', result, 'processed swagger')


def after_project_updated(hook, **kwargs):	    
    content_path = os.path.abspath(os.path.join(__file__, '..', '..', BUCKET_ROOT_DIRECTORY_NAME))
    upload_project_content(hook.context, content_path,
                           kwargs['args'].cognito_prod if 'args' in kwargs else None,
                           kwargs['args'].duration_seconds if 'args' in kwargs else constant.PROJECT_CGP_DEFAULT_EXPIRATION_SECONDS)


def upload_project_content(context, content_path, customer_cognito_id=None, expiration=constant.PROJECT_CGP_DEFAULT_EXPIRATION_SECONDS):
    if not os.path.isdir(content_path):
        raise HandledError('Cloud Gem Portal project content not found at {}.'.format(content_path))
    
    uploader = Uploader(
        context,
        bucket=context.stack.get_physical_resource_id(context.config.project_stack_id, AWS_S3_BUCKET_NAME),
        key=''
    )

    uploader.upload_dir(BUCKET_ROOT_DIRECTORY_NAME, content_path)
    write_bootstrap(context, customer_cognito_id, expiration)


def write_bootstrap(context, customer_cognito_id, expiration=constant.PROJECT_CGP_DEFAULT_EXPIRATION_SECONDS):
    project_resources = context.config.project_resources
        
    if not project_resources.has_key(constant.PROJECT_CGP_RESOURCE_NAME):
        raise HandledError(
            'You can not open the Cloud Gem Portal without having the Cloud Gem Portal gem installed in your project.')

    cgp_s3_resource = project_resources[constant.PROJECT_CGP_RESOURCE_NAME]
    stack_id = cgp_s3_resource['StackId']
    bucket_id = cgp_s3_resource['PhysicalResourceId']
    region = resource_manager.util.get_region_from_arn(stack_id)
    s3_client = context.aws.session.client('s3',region, config=Config(signature_version='s3v4'))
    user_pool_resource = project_resources[constant.PROJECT_RESOURCE_NAME_USER_POOL]
    identity_pool_resource = project_resources[constant.PROJECT_RESOURCE_NAME_IDENTITY_POOL]
    project_handler = project_resources[constant.PROJECT_RESOURCE_HANDLER_NAME]['PhysicalResourceId']
    project_config_bucket_id = context.config.configuration_bucket_name
    
    if 'CloudGemPortalApp' not in user_pool_resource['UserPoolClients']:
        credentials = context.aws.load_credentials()
        access_key = credentials.get(constant.DEFAULT_SECTION_NAME, constant.ACCESS_KEY_OPTION)
        raise HandledError('The Cognito user pool \'{}\' is missing the \'CloudGemPortalApp\' app client.  \
                Ensure the Lumberyard user \'{}\' with AWS access key identifier \'{}\' in the Lumberyard Credentials Manager \
                has the policy \'AmazonCognitoReadOnly\' attached and a project stack has been created (Lumberyard -> AWS -> Resource Manager).'
                .format(constant.PROJECT_RESOURCE_NAME_USER_POOL, context.config.user_default_profile, access_key))
    client_id = user_pool_resource['UserPoolClients']['CloudGemPortalApp']['ClientId']
    user_pool_id = user_pool_resource['PhysicalResourceId']
    identity_pool_id = identity_pool_resource['PhysicalResourceId']

    # create an administrator account if one is not present
    is_new_user, username, password = create_portal_administrator(context)

    cgp_bootstrap_config = {
        "clientId": client_id,
        "userPoolId": user_pool_id,
        "identityPoolId": identity_pool_id,
        "projectConfigBucketId": project_config_bucket_id,
        "region": region,
        "firstTimeUse": is_new_user,
        "cognitoProd": customer_cognito_id if customer_cognito_id != "''" else None,
        "projectPhysicalId": project_handler
    }

    if password is not None:
        context.view.create_admin(username, password,
                                  'The Cloud Gem Portal administrator account has been created.')

    content = get_index(s3_client, bucket_id)
    content = content.replace(get_bootstrap(s3_client, bucket_id),
                              '<script>{}{}</script>'.format(BOOTSTRAP_VARIABLE_NAME, json.dumps(cgp_bootstrap_config)))
    result = None
    try:
        # TODO: write to an unique name and configure bucket to auto delete these objects after 1 hour
        # the max allowed --duration-seconds value.
        result = s3_client.put_object(Bucket=bucket_id, Key=constant.PROJECT_CGP_ROOT_FILE, Body=content,ContentType='text/html')
    except ClientError as e:
        if e.response["Error"]["Code"] in ["AccessDenied"]:
            credentials = context.aws.load_credentials()
            access_key = credentials.get(constant.DEFAULT_SECTION_NAME, constant.ACCESS_KEY_OPTION)
            context.view._output_message("The Lumberyard user '{0}' associated with AWS IAM access key identifier '{1}' is missing PUT permissions on the S3 bucket '{2}'. Now attempting to use old Cloud Gem Portal pre-signed urls.\nHave the administrator grant the AWS user account with access key '{1}' S3 PUT permissions for bucket '{2}'".format(context.config.user_default_profile, access_key, bucket_id))
        else:
            raise HandledError("Could not write to the key '{}' in the S3 bucket '{}'.".format(constant.PROJECT_CGP_ROOT_FILE,bucket_id), e)

    if result == None or result['ResponseMetadata']['HTTPStatusCode'] == 200:
        context.view._output_message("The Cloud Gem Portal bootstrap information has been written successfully.")
    else:
        raise HandledError("The index.html cloud not be set in the S3 bucket '{}'.  This Cloud Gem Portal site will not load.".format(bucket_id))

    updateUserPoolEmailMessage(context, get_index_url(s3_client, bucket_id, expiration), project_config_bucket_id)


def get_index(s3_client, bucket_id):
    # Request the index file
    try:
        s3_index_obj_request = s3_client.get_object(Bucket=bucket_id, Key=constant.PROJECT_CGP_ROOT_FILE)
    except ClientError as e:
        raise HandledError(
            "Could not read from the key '{}' in the S3 bucket '{}'.".format(constant.PROJECT_CGP_ROOT_FILE, bucket_id),
            e)

    # Does the user have access to it?
    if s3_index_obj_request['ResponseMetadata']['HTTPStatusCode'] != 200:
        raise HandledError(
            "The user does not have access to the file index.html file.  This Cloud Gem Portal site will not load.")

    content = s3_index_obj_request['Body'].read().decode('utf-8')
    return content


def get_index_url(s3_client, bucket_id, expiration):
    app_url = base64.b64encode(__get_presigned_url(s3_client, bucket_id, constant.PROJECT_CGP_ROOT_APP, expiration))
    dep_url = base64.b64encode(__get_presigned_url(s3_client, bucket_id, constant.PROJECT_CGP_ROOT_APP_DEPENDENCIES, expiration))
    return '{}#{}&{}'.format(__get_presigned_url(s3_client, bucket_id, constant.PROJECT_CGP_ROOT_FILE, expiration),app_url,dep_url)


def get_bootstrap(s3_client, bucket_id):
    # Request the index file
    content = get_index(s3_client, bucket_id)
    match = re.search(BOOTSTRAP_REGEX_PATTERN, content, re.M | re.I)
    return match.group()


def create_portal_administrator(context):
    resource = context.config.project_resources[constant.PROJECT_RESOURCE_NAME_USER_POOL]
    stackid = resource['StackId']
    region = resource_manager.util.get_region_from_arn(stackid)
    user_pool_id = resource['PhysicalResourceId']
    client = context.aws.client('cognito-idp', region=region)
    administrator_name = 'administrator'
    is_new_user = False
    password = None

    try:
        response = client.admin_get_user(
            UserPoolId=user_pool_id,
            Username=administrator_name
        )
        user_exists = True
        if 'UserStatus' in response and response['UserStatus'] == 'FORCE_CHANGE_PASSWORD':
            is_new_user = True
    except ClientError:
        user_exists = False

    if not user_exists:
        # create the account if it does not
        random_str = ''.join(
            random.choice(string.ascii_uppercase + string.ascii_lowercase + string.digits) for _ in range(8))
        password = ''.join((random.choice(string.ascii_uppercase),
                            random.choice(string.ascii_lowercase),
                            random.choice(string.digits), '@', random_str))
        # shuffle password characters
        chars = list(password)
        random.shuffle(chars)
        password = ''.join(chars)

        try:
            response = client.admin_create_user(
                UserPoolId=user_pool_id,
                Username=administrator_name,
                TemporaryPassword=password
            )
            response = client.admin_add_user_to_group(
                UserPoolId=user_pool_id,
                Username=administrator_name,
                GroupName='administrator'
            )
            is_new_user = True
        except ClientError as e:
            credentials = context.aws.load_credentials()
            access_key = credentials.get(constant.DEFAULT_SECTION_NAME, constant.ACCESS_KEY_OPTION)
            raise HandledError(
                "Failed to create the administrator account.  Have your administrator run this option or verify the user account '{}' with access key '{}' has the policies ['cognito-idp:AdminCreateUser', 'cognito-idp:AdminAddUserToGroup'].".format(
                    context.config.user_default_profile, access_key), e)

    return is_new_user, administrator_name, password


def updateUserPoolEmailMessage(context, url, project_config_bucket_id):
    project_name_parts = project_config_bucket_id.split('-')
    project_name = project_name_parts[0]
    resource = context.config.project_resources[constant.PROJECT_RESOURCE_NAME_USER_POOL]
    stackid = resource['StackId']
    region = resource_manager.util.get_region_from_arn(stackid)
    user_pool_id = resource['PhysicalResourceId']
    client = context.aws.client('cognito-idp', region=region)

    email_invite_subject = "Your Amazon Lumberyard Cloud Gem Portal temporary password"
    email_invite_message = "Your Amazon Lumberyard Administrator has invited you to the project " + project_name + "'s <a href=" + url + ">Cloud Gem Portal</a>.<BR><BR>Username: {username} <BR>Temporary Password: {####}  <BR><BR>Cloud Gem Portal URL: " + url

    try:
        client.update_user_pool(
            UserPoolId=user_pool_id,
            EmailVerificationMessage="You or your Amazon Lumberyard Administrator has reset your password for the <a href=" + url + ">Cloud Gem Portal</a> on your project '" + project_name + "'.<BR><BR>You will need this code to change your password when you login next.<BR>Code: {####} <BR><BR>Cloud Gem Portal URL: " + url,
            EmailVerificationSubject='Your Amazon Lumberyard Cloud Gem Portal verification code',
            AdminCreateUserConfig={
                'InviteMessageTemplate': {
                    'EmailMessage': email_invite_message,
                    'EmailSubject': email_invite_subject
                },
                "AllowAdminCreateUserOnly": True
            },
            AutoVerifiedAttributes=['email']
        )
        context.view._output_message("The Cloud Gem Portal URL has been written to the Cognito user email template successfully.")
    except ClientError:
        return

# version constants
V_1_0_0 = Version('1.0.0')    
V_1_1_0 = Version('1.1.0')    
V_1_1_1 = Version('1.1.1')
V_1_1_2 = Version('1.1.2')
V_1_1_3 = Version('1.1.3')


def add_framework_version_update_writable_files(hook, from_version, to_version, writable_file_paths, **kwargs):

    # Repeat this pattern to add more upgrade processing:
    #
    # if from_version < VERSION_CHANGE_WAS_INTRODUCED:
    #     add files that may be written to

    # For now only need to update local-project-settings.json, which will always 
    # be writable when updating the framework version.
    pass


def before_framework_version_updated(hook, from_version, to_version, **kwargs):
    '''Called by the framework before updating the project's framework version.'''

    if from_version < V_1_1_1:
        hook.context.resource_groups.before_update_framework_version_to_1_1_1(from_version)

    if from_version < V_1_1_2:        
        hook.context.resource_groups.before_update_framework_version_to_1_1_2(from_version)

    if from_version < V_1_1_3:        
        hook.context.resource_groups.before_update_framework_version_to_1_1_3(from_version)
    # Repeat this pattern to add more upgrade processing:
    #
    # if from_version < VERSION_CHANGE_WAS_INTRODUCED:
    #     call an upgrade function (don't put upgrade logic in this function)


def after_framework_version_updated(hook, from_version, to_version, **kwargs):
    '''Called by the framework after updating the project's framework version.'''

    # Repeat this pattern to add more upgrade processing:
    #
    # if from_version < VERSION_CHANGE_WAS_INTRODUCED:
    #     call an upgrade function (don't put upgrade logic in this function)

    # currently don't have anything to do...
    pass

def __get_presigned_url(s3, bucket_id, key, expiration):
    presigned_url = s3.generate_presigned_url('get_object',
       Params={'Bucket': bucket_id, 'Key': key},
       ExpiresIn=expiration,
       HttpMethod="GET")
    return presigned_url


