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

import thread
import sys
import traceback

import project
import deployment
import mappings
import resource_group
import player_identity
import profile
import util
import importer
import function

from errors import HandledError
from context import Context
from metrics import MetricsContext
from view import ViewContext

from util import Args

from botocore.exceptions import NoCredentialsError
from botocore.exceptions import EndpointConnectionError
from botocore.exceptions import IncompleteReadError
from botocore.exceptions import ConnectionError
from botocore.exceptions import UnknownEndpointError

command_handlers = {
    'list-importable-resources': importer.list_aws_resources,
    'import-resource': importer.import_resource,
    'describe-project': project.describe,
    'create-project-stack': project.create_stack,
    'list-regions': project.get_regions,
    'update-mappings': mappings.force_update,
    'list-mappings': mappings.list,
    'list-deployments': deployment.list,
    'list-resource-groups': resource_group.list,
    'list-project-resources': project.list_project_resources,
    'list-deployment-resources': deployment.list_deployment_resources,
    'list-resource-group-resources': resource_group.list_resource_group_resources,
    'default-deployment': deployment.default,
    'default-profile': profile.default,
    'list-profiles': profile.list,
    'add-profile': profile.add,
    'remove-profile': profile.remove,
    'update-profile': profile.update,
    'rename-profile': profile.rename,
    'describe-project-stack': project.describe_stack,
    'describe-deployment-stack': deployment.describe_stack,
    'describe-resource-group-stack': resource_group.describe_stack,
    'upload-resources': deployment.upload_resources,
    'upload-lambda-code': function.upload_lambda_code,
    'update-project-stack': project.update_stack,
    'add-resource-group': resource_group.add,
    'disable-resource-group': resource_group.disable,
    'enable-resource-group': resource_group.enable,
    'create-deployment-stack': deployment.create_stack,
    'delete-deployment-stack' : deployment.delete_stack,
    'protect-deployment' : deployment.protect,
    'create-function-folder': resource_group.create_function_folder,
    'update-framework-version': project.update_framework_version
}


def execute(command, args):
    thread.start_new_thread(__execute, (command, args))


def __execute(command, args):

    requestId = args['request_id'] if 'request_id' in args else 0

    print 'command started - {} with args {}'.format(command, args)

    metricsInterface = MetricsContext('gui')
    metricsInterface.set_command_name(command)
    metricsInterface.submit_attempt()

    try:
        argsObj = Args(**args)

        argsObj.no_prompt = True
        argsObj.is_gui = True
        
        context = Context(metricsInterface,view_class=GuiViewContext)
        context.bootstrap(argsObj)
        context.initialize(argsObj)

        # Deprecated in 1.9. TODO: remove.
        context.hooks.call_module_handlers('cli-plugin-code/resource_commands.py','add_gui_commands', 
            args=[command_handlers, argsObj], 
            deprecated=True
        )
        context.hooks.call_module_handlers('cli-plugin-code/resource_commands.py', 'add_gui_view_commands', 
            args=[context.view], 
            deprecated=True
        )

        context.hooks.call_module_handlers('resource-manager-code/command.py','add_gui_commands', 
            kwargs={
                'handlers': command_handlers
            }
        )

        context.hooks.call_module_handlers('resource-manager-code/command.py', 'add_gui_view_commands', 
            kwargs={
                'view_context': context.view
            }
        )

        handler = command_handlers.get(command, None)
        if handler is None:
            raise HandledError('Unknown command: ' + command)

        if handler != project.update_framework_version:
            context.config.verify_framework_version()

        handler(context, argsObj)

        context.view.success()
        metricsInterface.submit_success()

    except HandledError as e:
        metricsInterface.submit_failure()
        msg = str(e)
        print 'command error   - {} when executing command {} with args {}.'.format(msg, command, args)
        args['view_output_function'](requestId, 'error', msg)
    except NoCredentialsError:
        metricsInterface.submit_failure()
        msg = 'No AWS credentials were provided.'
        print 'command error   - {} when executing command {} with args {}.'.format(msg, command, args)
        args['view_output_function'](requestId, 'error', msg)
    except (EndpointConnectionError, IncompleteReadError, ConnectionError, UnknownEndpointError) as e:
        metricsInterface.submit_failure()        
        msg = 'We were unable to contact your AWS endpoint.\nERROR: {0}'.format(e.message)
        print 'command error   - {} when executing command {} with args {}.'.format(msg, command, args)
        args['view_output_function'](requestId, 'error', msg)
    except:
        metricsInterface.submit_failure()
        info = sys.exc_info()
        msg = traceback.format_exception(*info)
        print 'command error   - {} when executing command {} with args {}.'.format(msg, command, args)
        args['view_output_function'](requestId, 'error', msg)

    print 'command finished - {} with args {}'.format(command, args)


class GuiViewContext(ViewContext):

    def __init__(self, context):
        super(GuiViewContext, self).__init__(context)

    def initialize(self, args):
        super(GuiViewContext, self).initialize(args)
        self.__view_output_function = args.view_output_function
        self.__requestId = args.request_id if args.request_id is not None else 0

    def project_stack_exists(self, value):
        self._output('project_stack_exists', value)

    def _output(self, key, value):
        print 'command output   - requestId: {}, key: {}, value: {}'.format(self.__requestId, key, value)
        self.__view_output_function(self.__requestId, str(key).encode('utf-8'), value)

    def success(self):
        self._output('success', 'Operation completed successfully.')

    def loading_file(self, path):
        pass # supressed in gui

    def importable_resource_list(self, importable_resources):
        self._output('importable-resource-list', 
            {
                'ImportableResources': importable_resources
            })

    def mapping_list(self, deployment_name, mappings, protected):
        self._output('mapping-list',
            {
                'DeploymentName': deployment_name,
                'Protected': protected,
                'Mappings': mappings
            })

    def confirm_writable_try_again(self, path_list):
        raise HandledError('Files not writable: {}'.format(path_list))

    def deployment_list(self, deployments):
        self._output('deployment-list',
            {
                'Deployments': deployments
            })

    def resource_group_list(self, deployment_name, resource_groups):
        self._output('resource-group-list',
            {
                'DeploymentName': deployment_name,
                'ResourceGroups': resource_groups
            })

    def project_resource_list(self, stack_id, resources_map):
        self.resource_list(stack_id, None, None, resources_map)

    def deployment_resource_list(self, stack_id, deployment_name, resources_map):
        self.resource_list(stack_id, deployment_name, None, resources_map)

    def resource_group_resource_list(self, stack_id, deployment_name, resource_group_name, resources_map):
        self.resource_list(stack_id, deployment_name, resource_group_name, resources_map)

    def resource_list(self, stack_id, deployment_name, resource_group_name, resource_map):

        resource_list = []
        for k,v in resource_map.iteritems():
            v['Name'] = k
            resource_list.append(v)

        self._output('resource-list',
            {
                'StackId': stack_id,
                'DeploymentName': deployment_name,
                'ResourceGroupName': resource_group_name,
                'Resources': resource_list
            })

    def profile_list(self, profiles, credentials_file_path):
        self._output('profile-list',
            {
                'CredentialsFilePath': credentials_file_path,
                'Profiles': profiles
            })

    def project_description(self, description):
        self._output('project-description',
            {
                'ProjectDetails': description
            })

    def describing_stack(self, stack_id):
        self._output_message('Retrieving description of stack {}.'.format(util.get_stack_name_from_arn(stack_id)))

    def project_stack_description(self, stack_description):
        self._output('project-stack-description',
            {
                'StackDescription': stack_description
            })

    def deployment_stack_description(self, deployment_name, stack_description):
        self._output('deployment-stack-description',
            {
                'DeploymentName': deployment_name,
                'StackDescription': stack_description
            })

    def resource_group_stack_description(self, deployment_name, resource_group_name, stack_description, user_defined_resource_count):
        self._output('resource-group-stack-description',
            {
                'DeploymentName': deployment_name,
                'ResourceGroupName': resource_group_name,
                'UserDefinedResourceCount': user_defined_resource_count,
                'StackDescription': stack_description
            })

    def stack_event_errors(self, errors, was_successful):
        self._output('stack-event-errors',
            {
                'Errors': errors
            })

    def supported_region_list(self, region_list):
        self._output('supported-region-list', region_list)

if __name__ == "__main__":
    sys.exit(main())