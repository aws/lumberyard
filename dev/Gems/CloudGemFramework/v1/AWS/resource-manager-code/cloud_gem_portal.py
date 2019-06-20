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
import re
import argparse
import subprocess
from resource_manager.errors import HandledError
from botocore.exceptions import ClientError
from resource_manager.uploader import ProjectUploader
from botocore.client import Config

def add_cli_commands(cgf_subparsers, addCommonArgs):

    subparser = cgf_subparsers.add_parser('open-cloud-gem-portal', aliases=['open-portal', 'cloud-gem-portal'],  help='Generate pre-signed url and launch the default browser with the pre-signed url.')
    subparser.add_argument('--show-url-only', action='store_true', required=False, help='Display the Cloud Gem Portal pre-signed url.', default=False)                        
    subparser.add_argument('--show-bootstrap-configuration', action='store_true', required=False, help='Display the Cloud Gem Portal bootstrap information.', default=False)                        
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

    subparser = cgf_subparsers.add_parser('gulp', help='Execute a gulp command from the CloudGemFramework\'s CloudGemPortal folder')
    subparser.add_argument('arglist', nargs=argparse.REMAINDER)
    addCommonArgs(subparser)
    subparser.set_defaults(func=cgp_gulp)

def open_portal(context, args):    
    project_resources = context.config.project_resources

    if not project_resources.has_key(constant.PROJECT_CGP_RESOURCE_NAME):
        raise HandledError('You can not open the Cloud Gem Portal without having the Cloud Gem Framework gem installed in your project.')
    
    cgp_s3_resource = project_resources[constant.PROJECT_CGP_RESOURCE_NAME]
    stackid = cgp_s3_resource['StackId']
    bucket_id = cgp_s3_resource['PhysicalResourceId']
    region = resource_manager.util.get_region_from_arn(stackid)
    cgp_static_url = update.get_index_url(context, region)

    if args.show_url_only:
        context.view._output_message(cgp_static_url)
    elif args.show_bootstrap_configuration: 
        s3_client = context.aws.session.client('s3',region, config=Config(region_name=region,signature_version='s3v4', s3={'addressing_style': 'virtual'}))
        context.view._output_message(__get_configuration(s3_client, bucket_id))
    else:
        webbrowser.open_new(cgp_static_url)

def __get_configuration(s3_client, bucket_id):
    return update.get_bootstrap(s3_client, bucket_id).replace("<script>", "").replace("</script>", "").replace(update.BOOTSTRAP_VARIABLE_NAME, "")

def create_portal_administrator(context, args):
    is_new_user, administrator_name, password = update.create_portal_administrator(context)
    if(args.silent_create_admin):
        return
    msg = 'The Cloud Gem Portal administrator account has been created.'
    if password is None:
        msg = "The administrator account already exists."
    context.view.create_admin(administrator_name, password, msg)

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

def cgp_gulp(context, args):
    gulpfile = 'gulpfile.js'
    gulpcmd = 'gulp'

    gulppath = 'Gems/CloudGemFramework/v1/Website/CloudGemPortal/'

    if not os.path.isfile(os.path.join(gulppath, gulpfile)):
        context.view._output_message('Could not find gulpfile in {}, exiting'.format(gulppath))
        return

    args.arglist.insert(0, gulpcmd)

    context.view._output_message('executing {} in {}'.format(args.arglist, gulppath))

    gulpproc = subprocess.Popen(args.arglist, cwd=gulppath, shell=True)

    gulpproc.wait()


