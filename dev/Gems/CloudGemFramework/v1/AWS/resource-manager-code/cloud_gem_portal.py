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
# $Revision: #1 $

import resource_manager.util
from resource_manager.errors import HandledError
import resource_manager.constant as constant
import json
import webbrowser
import re
import string
import random
import sys
import cStringIO
from botocore.exceptions import ClientError
from resource_manager.uploader import ProjectUploader
import os
from botocore.client import Config

import update

SRC_PATTERN = '''["|'].*{}.*["|']'''
BOOTSTRAP_VARIABLE_NAME = "var bootstrap ="

def add_cli_commands(cgf_subparsers, addCommonArgs):

    subparser = cgf_subparsers.add_parser('open-cloud-gem-portal', aliases=['open-portal', 'cloud-gem-portal'],  help='Generate pre-signed url and launch the default browser with the pre-signed url.')
    subparser.add_argument('--show-url-only', action='store_true', required=False, help='Display the Cloud Gem Portal pre-signed url.', default=False)
    subparser.add_argument('--show-configuration', action='store_true', required=False, help='Display the Cloud Gem Portal newly generated passphrase and iv.', default=False)
    subparser.add_argument('--show-current-configuration', action='store_true', required=False, help='Display the Cloud Gem Portal active passphrase and iv without generating a new encrypted payload.', default=False)
    subparser.add_argument('--duration-seconds', required=False, help='The number of seconds before the URL and temporary credentials will expire.', type=int, default=constant.PROJECT_CGP_DEFAULT_EXPIRATION_SECONDS)
    subparser.add_argument('--role', required=False, metavar='ROLE-NAME', help='Specifies an IAM role that will be assumed by the Cloud Gem Portal web site. Can be ProjectOwner, DeploymentOwner, or any other project or deployment access role. The credentials taken from the ~/.aws/credentials file must be able to asssume this role.')
    subparser.add_argument('--deployment', '-d', metavar='DEPLOYMENT-NAME', required=False, help='If ROLE-NAME is a deployment access role, this identifies the actual deployment. If not given, the default deployment for the project is used.')    
    subparser.add_argument('--cognito-dev', metavar='COGNITO-DEV', required=False, default=None, help='Optional parameter to pass additional the Lumberyard customer cognito identifier to the Cloud Gem Portal.')
    subparser.add_argument('--cognito-prod', metavar='COGNITO-PROD', required=False, default=None, help='Optional parameter to pass additional the Lumberyard customer cognito identifier to the Cloud Gem Portal.')
    subparser.add_argument('--cognito-test', metavar='COGNITO-TEST', required=False, default=None, help='Optional parameter to pass additional the Lumberyard customer cognito identifier to the Cloud Gem Portal.')
    subparser.add_argument('--silent-create-admin', action='store_true', required=False,
                           help='Create an administrator for the Cloud Gem Portal silently.',
                           default=False)
    addCommonArgs(subparser, no_assume_role = True)
    subparser.set_defaults(func=open_portal)

    subparser = cgf_subparsers.add_parser('upload-cloud-gem-portal', aliases=['upload-portal'], help='Upload all cloud gem portal content.')
    subparser.add_argument('--deployment', '-d', metavar='DEPLOYMENT-NAME', required=False, help='Name of the deployment for which portal content will be uploaded. If not specified the default deployment is updated.')
    subparser.add_argument('--resource-group', '-r', metavar='RESOURCE-GROUP-NAME', required=False, help='Name of the resource group for which portal content will be uploaded. The default is to upload portal content for all resource groups.')
    subparser.add_argument('--project', action='store_true', required=False, help='Update the project global portal content instead of updating deployment and resource group content.')
    addCommonArgs(subparser)
    subparser.set_defaults(func=upload_portal)

    subparser = cgf_subparsers.add_parser('create-portal-administrator', aliases=['create-admin'], help='Create the Cloud Gem Portal administrator account if none are present.')
    subparser.add_argument('--silent-create-admin', default=False, required=False, action='store_true', help='Run the command without outputs.')
    addCommonArgs(subparser)
    subparser.set_defaults(func=create_portal_administrator)

def open_portal(context, args):    
    project_resources = context.config.project_resources

    if not project_resources.has_key(constant.PROJECT_CGP_RESOURCE_NAME):
         raise HandledError('You can not open the Cloud Gem Portal without having the Cloud Gem Portal gem installed in your project.')
    
    project_config_bucket_id = context.config.configuration_bucket_name
    cgp_s3_resource = project_resources[constant.PROJECT_CGP_RESOURCE_NAME]
    stackid = cgp_s3_resource['StackId']
    bucket_id = cgp_s3_resource['PhysicalResourceId']
    expiration = args.duration_seconds if args.duration_seconds else constant.PROJECT_CGP_DEFAULT_EXPIRATION_SECONDS # default comes from argparse only on cli, gui call doesn't provide a default expiration
    region = resource_manager.util.get_region_from_arn(stackid)
    s3_client = context.aws.session.client('s3',region, config=Config(signature_version='s3v4'))
    user_pool_resource = project_resources[constant.PROJECT_RESOURCE_NAME_USER_POOL]
    identity_pool_resource = project_resources[constant.PROJECT_RESOURCE_NAME_IDENTITY_POOL]
    
    if 'CloudGemPortalApp' not in user_pool_resource['UserPoolClients']:  
        credentials = context.aws.load_credentials()      
        access_key = credentials.get(constant.DEFAULT_SECTION_NAME, constant.ACCESS_KEY_OPTION)
        raise HandledError('The Cognito user pool \'{}\' is missing the \'CloudGemPortalApp\' app client.  Ensure the Lumberyard user \'{}\' with AWS access key identifier \'{}\' in the Lumberyard Credentials Manager has the policy \'AmazonCognitoReadOnly\' attached and a project stack has been created (Lumberyard -> AWS -> Resource Manager).'.format(constant.PROJECT_RESOURCE_NAME_USER_POOL, context.config.user_default_profile, access_key))
    client_id = user_pool_resource['UserPoolClients']['CloudGemPortalApp']['ClientId']
    user_pool_id = user_pool_resource['PhysicalResourceId']
    identity_pool_id = identity_pool_resource['PhysicalResourceId']

    #create an administrator account if one is not present
    output = __validate_administrator_account(context, args)
    admin_account_created = __is_first_time_usage(output)

    #Request the index file
    try:
        s3_index_obj_request = s3_client.get_object(Bucket=bucket_id, Key= constant.PROJECT_CGP_ROOT_FILE)        
    except ClientError as e:
        raise HandledError("Could not read from the key '{}' in the S3 bucket '{}'.".format(constant.PROJECT_CGP_ROOT_FILE,bucket_id), e)

    #Does the user have access to it?
    if s3_index_obj_request['ResponseMetadata']['HTTPStatusCode'] != 200:
        raise HandledError("The user does not have access to the file index.html file.  This Cloud Gem Portal site will not load.")

    content = s3_index_obj_request['Body'].read().decode('utf-8')

    if args.show_current_configuration:
        try:
            cgp_current_bootstrap_config = s3_client.get_object(Bucket=bucket_id, Key=constant.PROJECT_CGP_ROOT_SUPPORT_FILE)
            cgp_current_bootstrap_config = cgp_current_bootstrap_config['Body'].read().decode('utf-8')
            context.view._output_message(cgp_current_bootstrap_config.replace(BOOTSTRAP_VARIABLE_NAME, ''))
            return
        except ClientError as e:
            raise HandledError("Could not read from the key '{}' in the S3 bucket '{}'.".format(constant.PROJECT_CGP_ROOT_SUPPORT_FILE, bucket_id), e)

    cgp_bootstrap_config = {
        "clientId": client_id,
        "userPoolId": user_pool_id,
        "identityPoolId": identity_pool_id,
        "projectConfigBucketId": project_config_bucket_id,
        "region": region,
        "firstTimeUse": admin_account_created,
        "cognitoDev" : args.cognito_dev if args.cognito_dev != "''" else None,
        "cognitoProd" : args.cognito_prod if args.cognito_prod != "''" else None,
        "cognitoTest" : args.cognito_test if args.cognito_test != "''" else None
    }

    content = set_presigned_urls(content, bucket_id, s3_client, expiration, region)
    result = None
    try:
        # TODO: write to an unique name and configure bucket to auto delete these objects after 1 hour
        # the max allowed --duration-seconds value.
        s3_client.put_object(Bucket=bucket_id, Key=constant.PROJECT_CGP_ROOT_SUPPORT_FILE, Body="var bootstrap = {}".format(json.dumps(cgp_bootstrap_config)),ContentType='text/html')
        result = s3_client.put_object(Bucket=bucket_id, Key=constant.PROJECT_CGP_ROOT_FILE, Body=content,ContentType='text/html')
    except ClientError as e:
        if e.response["Error"]["Code"] in ["AccessDenied"]:
            credentials = context.aws.load_credentials()      
            access_key = credentials.get(constant.DEFAULT_SECTION_NAME, constant.ACCESS_KEY_OPTION)
            context.view._output_message("The Lumberyard user '{0}' associated with AWS IAM access key identifier '{1}' is missing PUT permissions on the S3 bucket '{2}'. Now attempting to use old Cloud Gem Portal pre-signed urls.\nHave the administrator grant the AWS user account with access key '{1}' S3 PUT permissions for bucket '{2}'".format(context.config.user_default_profile, access_key, bucket_id))
        else:
            raise HandledError("Could write to the key '{}' in the S3 bucket '{}'.".format(constant.PROJECT_CGP_ROOT_FILE,bucket_id), e)

    if result == None or result['ResponseMetadata']['HTTPStatusCode'] == 200:
        if result != None and not set_bucket_cors(context, project_config_bucket_id, region):
            raise HandledError("Warning: the Cross Origin Resource Sharing (CORS) policy cloud not be set:  Access Denied.  This may prevent the Cloud Gem Portal from accessing the projects project-settings.json file.")

        #generate presigned url
        secured_url = __get_presigned_url(s3_client, bucket_id, constant.PROJECT_CGP_ROOT_FILE, expiration)

        __updateUserPoolEmailMessage(context, secured_url, project_config_bucket_id)
        if args.show_configuration:
            context.view._output_message(json.dumps(cgp_bootstrap_config))

        if args.show_url_only:
            context.view._output_message(secured_url)
        else:
            webbrowser.open_new(secured_url)
    else:
        raise HandledError("The index.html cloud not be set in the S3 bucket '{}'.  This Cloud Gem Portal site will not load.".format(bucket_id))

def set_presigned_urls(html, bucket_id, s3, expiration, region):
    srcs = ['bundles/dependencies.bundle.js',
            'bundles/app.bundle.js',
            'cgp_bootstrap.js',
            'config.js',
            constant.PROJECT_CGP_BOOTSTRAP_FILENAME
            ]

    for src in srcs:
        secure_url = __get_presigned_url(s3, bucket_id, "{}/{}".format(constant.PROJECT_CGP_ROOT_FOLDER,src), expiration)
        pattern = SRC_PATTERN.format(src.replace("/","\/"))
        matchs = re.findall(pattern, html, re.M | re.I)
        for match in matchs:
            html = html.replace(match,"'"+secure_url+"'") 

    return html

def set_bucket_cors(context, bucket, region):
    s3 = context.aws.session.resource('s3', region, config=Config(signature_version='s3v4'))
    bucket = s3.Bucket(bucket)
    cors = bucket.Cors()

    try:
        cors_rules = cors.cors_rules[0]
        allowed_headers = cors_rules['AllowedHeaders']
        if 'x-amz-date' not in allowed_headers:
            allowed_headers.append('x-amz-date')
        if 'x-amz-user-agent' not in allowed_headers:
            allowed_headers.append('x-amz-user-agent')
        if 'authorization' not in allowed_headers:
            allowed_headers.append('authorization')
        if 'x-amz-security-token' not in allowed_headers:
            allowed_headers.append('x-amz-security-token')
        if 'x-amz-content-sha256' not in allowed_headers:
            allowed_headers.append('x-amz-content-sha256')
    except ClientError as e:
        #No CORS policy exists.  Let's create one.
        cors_rules = {
            'AllowedMethods': ['GET'],
            'AllowedOrigins': ['*'],
            'MaxAgeSeconds': 3000,
        }
        allowed_headers = []
        allowed_headers.append('x-amz-date')
        allowed_headers.append('x-amz-user-agent')
        allowed_headers.append('x-amz-security-token')
        allowed_headers.append('authorization')
        allowed_headers.append('x-amz-content-sha256')

    response = cors.put(CORSConfiguration={
        'CORSRules': [
            {
                'AllowedHeaders': allowed_headers,
                'AllowedMethods': cors_rules['AllowedMethods'],
                'AllowedOrigins': cors_rules['AllowedOrigins'],                
                'MaxAgeSeconds':  cors_rules['MaxAgeSeconds']
            },
        ]
    })
    return response['ResponseMetadata']['HTTPStatusCode'] == 200

def upload_portal(context, args):

    if args.project:

        if args.deployment or args.resource_group:
            raise HandledError('The --project option cannot be used with the --deployment or --resource-group options.')

        content_path = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', update.BUCKET_ROOT_DIRECTORY_NAME))
        update.upload_project_content(context, content_path)

    else:

        deployment_name = args.deployment
        if deployment_name is None:
            deployment_name = context.config.default_deployment

        if deployment_name not in context.config.deployment_names:
            raise HandledError('There is no {} deployment.'.format(deployment_name))

        project_uploader = ProjectUploader(context)
        deployment_uploader = project_uploader.get_deployment_uploader(deployment_name)

        resource_group_name = args.resource_group
        if resource_group_name is not None:
            resource_group = context.resource_groups.get(resource_group_name)
            resource_group.update_cgp_code(deployment_uploader.get_resource_group_uploader(resource_group.name))
        else:
            for resource_group in context.resource_groups.values():
                resource_group.update_cgp_code(deployment_uploader.get_resource_group_uploader(resource_group.name))

def create_portal_administrator(context, args):
    resource = context.config.project_resources[constant.PROJECT_RESOURCE_NAME_USER_POOL]
    stackid = resource['StackId']
    region = resource_manager.util.get_region_from_arn(stackid)
    user_pool_id = resource['PhysicalResourceId']
    client = context.aws.client('cognito-idp', region=region)
    administrator_name = 'administrator'

    try:
        response = client.admin_get_user(
            UserPoolId=user_pool_id,
            Username=administrator_name
        )
        user_exists = True
    except Exception as e:
        user_exists = False

    if not user_exists:
        # create the account if it does not
        random_str = ''.join(random.choice(string.ascii_uppercase + string.ascii_lowercase + string.digits) for _ in range(8))
        password = ''.join((random.choice(string.ascii_uppercase),
                            random.choice(string.ascii_lowercase),
                            random.choice(string.digits), '@', random_str))
        #shuffle password characters
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
        except Exception as e:
            credentials = context.aws.load_credentials()      
            access_key = credentials.get(constant.DEFAULT_SECTION_NAME, constant.ACCESS_KEY_OPTION)            
            raise HandledError("Failed to create the administrator account.  Have your administrator run this option or verify the user account '{}' with access key '{}' has the policies ['cognito-idp:AdminCreateUser', 'cognito-idp:AdminAddUserToGroup'].".format(context.config.user_default_profile,access_key), e)
            
        if not args.silent_create_admin:
            context.view.create_admin(administrator_name, password, 'The Cloud Gem Portal administrator account has been created.')


def __validate_administrator_account(context, args):

    resource = context.config.project_resources[constant.PROJECT_RESOURCE_NAME_USER_POOL]
    stackid = resource['StackId']
    region = resource_manager.util.get_region_from_arn(stackid)
    user_pool_id = resource['PhysicalResourceId']
    client = context.aws.client('cognito-idp', region=region)
    administrator_name = 'administrator'

    stdout = sys.stdout
    stream = cStringIO.StringIO()
    sys.stdout = stream

    create_portal_administrator(context, args)

    sys.stdout = stdout

    if not args.silent_create_admin:
        print stream.getvalue()

    response = client.admin_get_user(
        UserPoolId=user_pool_id,
        Username=administrator_name
    )

    return response

def __is_first_time_usage(response):
    if 'UserStatus' in response and response['UserStatus'] == 'FORCE_CHANGE_PASSWORD':
        return True
    return False

def __updateUserPoolEmailMessage(context, url, project_config_bucket_id):
    project_name_parts = project_config_bucket_id.split('-')
    project_name = project_name_parts[0]
    resource = context.config.project_resources[constant.PROJECT_RESOURCE_NAME_USER_POOL]
    stackid = resource['StackId']
    region = resource_manager.util.get_region_from_arn(stackid)
    user_pool_id = resource['PhysicalResourceId']
    client = context.aws.client('cognito-idp', region=region)

    email_invite_subject = "Your Amazon Lumberyard Cloud Gem Portal temporary password"
    email_invite_message = "Your Amazon Lumberyard Administrator has invited you to the project '" + project_name + "' <a href="+url+">Cloud Gem Portal</a>.<BR><BR>Username: {username} <BR>Temporary Password: {####}  <BR><BR>Cloud Gem Portal URL: " + url

    try:
        client.update_user_pool(
            UserPoolId=user_pool_id,
            EmailVerificationMessage="You or your Amazon Lumberyard Administrator has reset your password for the <a href="+url+">Cloud Gem Portal</a> on your project '" + project_name + "'.<BR><BR>You will need this code to change your password when you login next.<BR>Code: {####} <BR><BR>Cloud Gem Portal URL: " + url,
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
    except ClientError:
        return

def __get_presigned_url(s3, bucket_id, key, expiration):
    presigned_url = s3.generate_presigned_url('get_object',
       Params={'Bucket': bucket_id, 'Key': key},
       ExpiresIn=expiration,
       HttpMethod="GET")
    return presigned_url
