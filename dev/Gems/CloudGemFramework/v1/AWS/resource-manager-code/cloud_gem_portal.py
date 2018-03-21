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
import resource_manager_common.constant as constant
import webbrowser
import base64
import os
import update
from resource_manager.errors import HandledError
from botocore.exceptions import ClientError
from resource_manager.uploader import ProjectUploader
from botocore.client import Config


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
    subparser.add_argument('--duration-seconds', required=False,
                           help='The number of seconds before the URL and temporary credentials will expire.', type=int,
                           default=constant.PROJECT_CGP_DEFAULT_EXPIRATION_SECONDS)
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

    cgp_s3_resource = project_resources[constant.PROJECT_CGP_RESOURCE_NAME]
    stackid = cgp_s3_resource['StackId']
    bucket_id = cgp_s3_resource['PhysicalResourceId']
    expiration = args.duration_seconds
    region = resource_manager.util.get_region_from_arn(stackid)
    s3_client = context.aws.session.client('s3',region, config=Config(signature_version='s3v4'))

    if args.show_current_configuration:
        try:
            context.view._output_message(__get_configuration(s3_client, bucket_id))
            return
        except ClientError as e:
            raise HandledError("Could not read from the key '{}' in the S3 bucket '{}'.".format(constant.PROJECT_CGP_ROOT_SUPPORT_FILE, bucket_id), e)

    result = None
    if result is None or result['ResponseMetadata']['HTTPStatusCode'] == 200:
        # generate presigned url
        index_url = update.get_index_url(s3_client, bucket_id, expiration)

        if args.show_configuration:
            context.view._output_message(__get_configuration(s3_client, bucket_id))

        if args.show_url_only:
            context.view._output_message(index_url)
        else:
            webbrowser.open_new(index_url)
    else:
        raise HandledError("The index.html cloud not be set in the S3 bucket '{}'.  This Cloud Gem Portal site will not load.".format(bucket_id))

def __get_configuration(s3_client, bucket_id):
    return update.get_bootstrap(s3_client, bucket_id).replace("<script>", "").replace("</script>", "").replace(update.BOOTSTRAP_VARIABLE_NAME, "")

def create_portal_administrator(context, args):
    is_new_user, administrator_name, password = update.create_portal_administrator(context)
    if(args.silent_create_admin):
        return
    context.view.create_admin(administrator_name, password, 'The Cloud Gem Portal administrator account has been created.')

def upload_portal(context, args):
    if args.project:

        if args.deployment or args.resource_group:
            raise HandledError('The --project option cannot be used with the --deployment or --resource-group options.')

        content_path = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', update.BUCKET_ROOT_DIRECTORY_NAME))
        update.upload_project_content(context, content_path, expiration=args.duration_seconds)
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



