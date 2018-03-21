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
# $Revision: #3 $

import unittest
import mock
import json

from cgf_utils.test import mock_aws
from test_case import ResourceHandlerTestCase

from resource_manager_common import stack_info
from resource_types import Custom_AccessControl

def merge_dicts(*args):
    result = {}
    for arg in args:
        result.update(arg)
    return result


class Custom_AccessControlTestCase(ResourceHandlerTestCase):

    def __init__(self, *args, **kwargs):
        super(Custom_AccessControlTestCase, self).__init__(*args, **kwargs)

    def make_problem_reporting_side_effect(self, problems_caused, return_values = None):
        index = [0] # in list to avoid reference before assignment error in side effect function below
        def side_effect(*args):
            # assumes that the last argument is the "problems" list to which problems, if given, will be added
            if problems_caused and index[0] < len(problems_caused):
                problems = args[len(args)-1]
                problems.append(problems_caused[index[0]])
            if return_values:
                return_value = return_values[index[0]]
            else:
                return_value = None
            index[0] = index[0] + 1
            return return_value
        return side_effect

    ANY_PROBLEM_LIST = ResourceHandlerTestCase.AnyInstance(Custom_AccessControl.ProblemList)


class UnitTest_CloudGemFramework_ProjectResourceHandler_Custom_AccessControl_handler(Custom_AccessControlTestCase):

    @mock.patch('cgf_utils.custom_resource_response.success_response')
    @mock.patch('resource_manager_common.stack_info.StackInfoManager.get_stack_info')
    @mock.patch('resource_types.Custom_AccessControl._apply_resource_group_access_control')
    def __do_stack_type_resource_group_test(self, 
            request_type, 
            problems,
            mock_apply_resource_group_access_control, 
            mock_get_stack_info, 
            mock_succeed):

        event = self.make_event(request_type)

        resource_group = mock_get_stack_info.return_value
        resource_group.stack_type = resource_group.STACK_TYPE_RESOURCE_GROUP

        mock_apply_resource_group_access_control.side_effect = self.make_problem_reporting_side_effect(problems)

        if problems:
            with self.assertRaises(RuntimeError):
                Custom_AccessControl.handler(event, self.CONTEXT)
        else:
            Custom_AccessControl.handler(event, self.CONTEXT)

        mock_get_stack_info.assert_called_with(self.STACK_ARN)
        mock_apply_resource_group_access_control.assert_called_with(request_type, resource_group, self.ANY_PROBLEM_LIST)

        if problems:
            mock_succeed.assert_not_called()
        else:
            mock_succeed.assert_called_with({}, self.PHYSICAL_RESOURCE_ID)


    def test_stack_type_resource_group_create_with_no_problems(self):
        self.__do_stack_type_resource_group_test('Create', [])


    def test_stack_type_resource_group_update_with_no_problems(self):
        self.__do_stack_type_resource_group_test('Update', [])


    def test_stack_type_resource_group_delete_with_no_problems(self):
        self.__do_stack_type_resource_group_test('Delete', [])


    def test_stack_type_resource_group_create_with_problems(self):
        self.__do_stack_type_resource_group_test('Create', ['Problem'])


    def test_stack_type_resource_group_update_with_problems(self):
        self.__do_stack_type_resource_group_test('Update', ['Problem'])


    def test_stack_type_resource_group_delete_with_problems(self):
        self.__do_stack_type_resource_group_test('Delete', ['Problem'])


    @mock.patch('cgf_utils.custom_resource_response.success_response')
    @mock.patch('resource_manager_common.stack_info.StackInfoManager.get_stack_info')
    @mock.patch('resource_types.Custom_AccessControl._apply_deployment_access_control')
    def __do_stack_type_deployment_access_test(self, 
            request_type,
            problems,
            mock_apply_deployment_access_control, 
            mock_get_stack_info, 
            mock_succeed):

        event = self.make_event(request_type)

        deployment_access = mock_get_stack_info.return_value
        deployment_access.stack_type = deployment_access.STACK_TYPE_DEPLOYMENT_ACCESS

        mock_apply_deployment_access_control.side_effect = self.make_problem_reporting_side_effect(problems)

        if problems:
            with self.assertRaises(RuntimeError):
                Custom_AccessControl.handler(event, self.CONTEXT)
        else:
            Custom_AccessControl.handler(event, self.CONTEXT)

        mock_get_stack_info.assert_called_with(self.STACK_ARN)
        mock_apply_deployment_access_control.assert_called_with(request_type, deployment_access, self.ANY_PROBLEM_LIST)

        if problems:
            mock_succeed.assert_not_called()
        else:
            mock_succeed.assert_called_with({}, self.PHYSICAL_RESOURCE_ID)


    def test_stack_type_deployment_access_create_with_no_problems(self):
        self.__do_stack_type_deployment_access_test('Create', [])


    def test_stack_type_deployment_access_update_with_no_problems(self):
        self.__do_stack_type_deployment_access_test('Update', [])


    def test_stack_type_deployment_access_delete_with_no_problems(self):
        self.__do_stack_type_deployment_access_test('Delete', [])


    def test_stack_type_deployment_access_create_with_problems(self):
        self.__do_stack_type_deployment_access_test('Create', ['Problem'])


    def test_stack_type_deployment_access_update_with_problems(self):
        self.__do_stack_type_deployment_access_test('Update', ['Problem'])


    def test_stack_type_deployment_access_delete_with_problems(self):
        self.__do_stack_type_deployment_access_test('Delete', ['Problem'])


    @mock.patch('cgf_utils.custom_resource_response.success_response')
    @mock.patch('resource_manager_common.stack_info.StackInfoManager.get_stack_info')
    @mock.patch('resource_types.Custom_AccessControl._apply_project_access_control')
    def __do_stack_type_project_test(self, 
            request_type,
            problems,
            mock_apply_project_access_control, 
            mock_get_stack_info, 
            mock_succeed):

        event = self.make_event(request_type)

        project = mock_get_stack_info.return_value
        project.stack_type = project.STACK_TYPE_PROJECT

        mock_apply_project_access_control.side_effect = self.make_problem_reporting_side_effect(problems)

        if problems:
            with self.assertRaises(RuntimeError):
                Custom_AccessControl.handler(event, self.CONTEXT)
        else:
            Custom_AccessControl.handler(event, self.CONTEXT)

        mock_get_stack_info.assert_called_with(self.STACK_ARN)
        mock_apply_project_access_control.assert_called_with(request_type, project, self.ANY_PROBLEM_LIST)

        if problems:
            mock_succeed.assert_not_called()
        else:
            mock_succeed.assert_called_with({}, self.PHYSICAL_RESOURCE_ID)


    def test_stack_type_project_create_with_no_problems(self):
        self.__do_stack_type_project_test('Create', [])


    def test_stack_type_project_update_with_no_problems(self):
        self.__do_stack_type_project_test('Update', [])


    def test_stack_type_project_delete_with_no_problems(self):
        self.__do_stack_type_project_test('Delete', [])


    def test_stack_type_project_create_with_problems(self):
        self.__do_stack_type_project_test('Create', ['Problem'])


    def test_stack_type_project_update_with_problems(self):
        self.__do_stack_type_project_test('Update', ['Problem'])


    def test_stack_type_project_delete_with_problems(self):
        self.__do_stack_type_project_test('Delete', ['Problem'])


    @mock.patch('cgf_utils.custom_resource_response.success_response')
    @mock.patch('resource_manager_common.stack_info.StackInfoManager.get_stack_info')
    def __do_stack_type_deployment_test(self, 
            request_type,                                     
            mock_get_stack_info, 
            mock_succeed):

        event = self.make_event(request_type)

        deployment = mock_get_stack_info.return_value
        deployment.stack_type = deployment.STACK_TYPE_DEPLOYMENT

        with self.assertRaises(RuntimeError):
            Custom_AccessControl.handler(event, self.CONTEXT)

        mock_succeed.assert_not_called()
        mock_get_stack_info.assert_called_with(self.STACK_ARN)


    def test_stack_type_deployment_create(self):
        self.__do_stack_type_deployment_test('Create')


    def test_stack_type_deployment_update(self):
        self.__do_stack_type_deployment_test('Update')


    def test_stack_type_deployment_delete(self):
        self.__do_stack_type_deployment_test('Delete')


    def test_unexpected_request_type(self):

        event = self.make_event('Unexpected')

        with self.assertRaises(RuntimeError):
            Custom_AccessControl.handler(event, self.CONTEXT)


class UnitTest_CloudGemFramework_ProjectResourceHandler_Custom_AccessControl_apply_resource_group_access_control(Custom_AccessControlTestCase):

    RESOURCE_GROUP = mock.MagicMock()
    REQUEST_TYPE = mock.MagicMock()

    RESOURCE_GROUP_ROLE_MAPPINGS = mock.MagicMock()
    DEPLOYMENT_ACCESS_ROLE_MAPPINGS = mock.MagicMock()
    PROJECT_ROLE_MAPPINGS = mock.MagicMock()

    @mock.patch('resource_types.Custom_AccessControl._get_resource_group_policy_name')
    @mock.patch('resource_types.Custom_AccessControl._get_permissions')
    @mock.patch('resource_types.Custom_AccessControl._get_explicit_role_mappings')
    @mock.patch('resource_types.Custom_AccessControl._get_implicit_role_mappings')
    @mock.patch('resource_types.Custom_AccessControl._update_roles', return_value=True)
    def test_with_no_problems(self, 
            mock_update_roles,
            mock_get_implicit_role_mappings, 
            mock_get_explicit_role_mappings, 
            mock_get_permissions, 
            mock_get_resource_group_policy_name):

        mock_get_explicit_role_mappings.side_effect = [ self.DEPLOYMENT_ACCESS_ROLE_MAPPINGS, self.PROJECT_ROLE_MAPPINGS ]
        mock_get_implicit_role_mappings.side_effect = [ self.RESOURCE_GROUP_ROLE_MAPPINGS ]

        policy_name = mock_get_resource_group_policy_name.return_value

        permissions = mock_get_permissions.return_value

        expected_problems = []

        problems = Custom_AccessControl.ProblemList()

        Custom_AccessControl._apply_resource_group_access_control(self.REQUEST_TYPE, self.RESOURCE_GROUP, problems)

        self.assertEquals(len(problems), len(expected_problems))

        mock_get_resource_group_policy_name.assert_has_calls([
            mock.call(self.RESOURCE_GROUP)])
        
        mock_get_permissions.assert_has_calls([
            mock.call(self.RESOURCE_GROUP, problems)])

        mock_get_explicit_role_mappings.assert_has_calls([
            mock.call(self.RESOURCE_GROUP.deployment.deployment_access, problems),
            mock.call(self.RESOURCE_GROUP.deployment.project, problems)])

        mock_get_implicit_role_mappings.assert_has_calls([
            mock.call(self.RESOURCE_GROUP, problems)])

        mock_update_roles.assert_has_calls([
            mock.call(self.REQUEST_TYPE, policy_name, permissions, self.RESOURCE_GROUP_ROLE_MAPPINGS),
            mock.call(self.REQUEST_TYPE, policy_name, permissions, self.DEPLOYMENT_ACCESS_ROLE_MAPPINGS),
            mock.call(self.REQUEST_TYPE, policy_name, permissions, self.PROJECT_ROLE_MAPPINGS)])


    @mock.patch('resource_types.Custom_AccessControl._get_resource_group_policy_name')
    @mock.patch('resource_types.Custom_AccessControl._get_permissions')
    @mock.patch('resource_types.Custom_AccessControl._get_explicit_role_mappings')
    @mock.patch('resource_types.Custom_AccessControl._get_implicit_role_mappings')
    @mock.patch('resource_types.Custom_AccessControl._update_roles', return_value=True)
    def test_with_no_deployment_access_stack(self, 
            mock_update_roles,
            mock_get_implicit_role_mappings, 
            mock_get_explicit_role_mappings, 
            mock_get_permissions, 
            mock_get_resource_group_policy_name):

        mock_get_explicit_role_mappings.side_effect = [ self.PROJECT_ROLE_MAPPINGS ]
        mock_get_implicit_role_mappings.side_effect = [ self.RESOURCE_GROUP_ROLE_MAPPINGS ]

        policy_name = mock_get_resource_group_policy_name.return_value

        permissions = mock_get_permissions.return_value

        expected_problems = []

        problems = Custom_AccessControl.ProblemList()

        resource_group = mock.MagicMock()
        resource_group.deployment.deployment_access = None

        Custom_AccessControl._apply_resource_group_access_control(self.REQUEST_TYPE, resource_group, problems)

        self.assertEquals(len(problems), len(expected_problems))

        mock_get_resource_group_policy_name.assert_has_calls([
            mock.call(resource_group)])
        
        mock_get_permissions.assert_has_calls([
            mock.call(resource_group, problems)])

        mock_get_explicit_role_mappings.assert_has_calls([
            mock.call(resource_group.deployment.project, problems)])

        self.assertEquals(mock_get_explicit_role_mappings.call_count, 1)

        mock_get_implicit_role_mappings.assert_has_calls([
            mock.call(resource_group, problems)])

        mock_update_roles.assert_has_calls([
            mock.call(self.REQUEST_TYPE, policy_name, permissions, self.RESOURCE_GROUP_ROLE_MAPPINGS),
            mock.call(self.REQUEST_TYPE, policy_name, permissions, self.PROJECT_ROLE_MAPPINGS)])

        self.assertEquals(mock_update_roles.call_count, 2)


    @mock.patch('resource_types.Custom_AccessControl._get_resource_group_policy_name')
    @mock.patch('resource_types.Custom_AccessControl._get_permissions')
    @mock.patch('resource_types.Custom_AccessControl._get_explicit_role_mappings')
    @mock.patch('resource_types.Custom_AccessControl._get_implicit_role_mappings')
    @mock.patch('resource_types.Custom_AccessControl._update_roles')
    def test_with_with_problems(self, 
            mock_update_roles,
            mock_get_implicit_role_mappings, 
            mock_get_explicit_role_mappings, 
            mock_get_permissions, 
            mock_get_resource_group_policy_name):

        resource_group_implicit_role_mapping_problem = mock.MagicMock()

        deployment_access_role_mapping_problem = mock.MagicMock()
        project_role_mapping_problem = mock.MagicMock()

        mock_get_explicit_role_mappings_return_values = [ self.DEPLOYMENT_ACCESS_ROLE_MAPPINGS, self.PROJECT_ROLE_MAPPINGS ]
        mock_get_explicit_role_mappings_problems = [ deployment_access_role_mapping_problem, project_role_mapping_problem ]

        mock_get_explicit_role_mappings.side_effect = self.make_problem_reporting_side_effect(
            mock_get_explicit_role_mappings_problems,
            mock_get_explicit_role_mappings_return_values)

        mock_get_implicit_role_mappings.side_effect = self.make_problem_reporting_side_effect(
            [ resource_group_implicit_role_mapping_problem ],
            [ self.RESOURCE_GROUP_ROLE_MAPPINGS ])

        policy_name = mock_get_resource_group_policy_name.return_value

        permissions = mock.MagicMock()
        mock_get_permissions_problem = mock.MagicMock()
        mock_get_permissions.side_effect = self.make_problem_reporting_side_effect(
            [ mock_get_permissions_problem ],
            [ permissions ])

        expected_problems = [mock_get_permissions_problem, resource_group_implicit_role_mapping_problem]
        expected_problems.extend(mock_get_explicit_role_mappings_problems)

        problems = Custom_AccessControl.ProblemList()

        Custom_AccessControl._apply_resource_group_access_control(self.REQUEST_TYPE, self.RESOURCE_GROUP, problems)

        self.assertEquals(len(problems), len(expected_problems))

        mock_get_resource_group_policy_name.assert_has_calls([
            mock.call(self.RESOURCE_GROUP)])
        
        mock_get_permissions.assert_has_calls([
            mock.call(self.RESOURCE_GROUP, problems)])

        mock_get_explicit_role_mappings.assert_has_calls([
            mock.call(self.RESOURCE_GROUP.deployment.deployment_access, problems),
            mock.call(self.RESOURCE_GROUP.deployment.project, problems)])

        mock_get_implicit_role_mappings.assert_has_calls([
            mock.call(self.RESOURCE_GROUP, problems)])

        mock_update_roles.assert_not_called()


class UnitTest_CloudGemFramework_ProjectResourceHandler_Custom_AccessControl_apply_deployment_access_control(Custom_AccessControlTestCase):

    @mock.patch('resource_types.Custom_AccessControl._get_resource_group_policy_name')
    @mock.patch('resource_types.Custom_AccessControl._get_permissions')
    @mock.patch('resource_types.Custom_AccessControl._get_explicit_role_mappings')
    @mock.patch('resource_types.Custom_AccessControl._update_roles')
    def test_with_no_problems(self, 
            mock_update_roles,
            mock_get_explicit_role_mappings, 
            mock_get_permissions, 
            mock_get_resource_group_policy_name):

        request_type = mock.MagicMock()

        policy_name_1 = mock.MagicMock()
        policy_name_2 = mock.MagicMock()

        mock_get_resource_group_policy_name.side_effect = [ policy_name_1, policy_name_2 ]

        permissions_1 = mock.MagicMock()
        permissions_2 = mock.MagicMock()
        mock_get_permissions.side_effect = [ permissions_1, permissions_2 ]

        resource_group_1 = mock.MagicMock()
        resource_group_2 = mock.MagicMock()
        deployment_access = mock.MagicMock()
        deployment_access.deployment.resource_groups = [ resource_group_1, resource_group_2 ]

        explicit_role_mappings = mock_get_explicit_role_mappings.return_value

        expected_problems = []

        problems = Custom_AccessControl.ProblemList()

        Custom_AccessControl._apply_deployment_access_control(request_type, deployment_access, problems)

        self.assertEquals(len(problems), len(expected_problems))

        mock_get_explicit_role_mappings.assert_called_with(deployment_access, problems)

        mock_get_resource_group_policy_name.assert_has_calls([
            mock.call(resource_group_1),
            mock.call(resource_group_2)])

        mock_get_permissions.assert_has_calls([
            mock.call(resource_group_1, problems),
            mock.call(resource_group_2, problems)])

        mock_update_roles.assert_has_calls([
            mock.call(request_type, policy_name_1, permissions_1, explicit_role_mappings),
            mock.call(request_type, policy_name_2, permissions_2, explicit_role_mappings)],
            any_order = True)


    @mock.patch('resource_types.Custom_AccessControl._get_resource_group_policy_name')
    @mock.patch('resource_types.Custom_AccessControl._get_permissions')
    @mock.patch('resource_types.Custom_AccessControl._get_explicit_role_mappings')
    @mock.patch('resource_types.Custom_AccessControl._update_roles')
    def test_with_problems(self, 
            mock_update_roles,
            mock_get_explicit_role_mappings, 
            mock_get_permissions, 
            mock_get_resource_group_policy_name):

        request_type = mock.MagicMock()

        policy_name_1 = mock.MagicMock()
        policy_name_2 = mock.MagicMock()

        mock_get_resource_group_policy_name.side_effect = [ policy_name_1, policy_name_2 ]

        permissions_1 = mock.MagicMock()
        permissions_2 = mock.MagicMock()

        mock_get_permissions_return_values = [ permissions_1, permissions_2 ]

        mock_get_permissions_problem_1 = mock.MagicMock()
        mock_get_permissions_problem_2 = mock.MagicMock()
        mock_get_permissions_problems = [ mock_get_permissions_problem_1, mock_get_permissions_problem_2 ]

        mock_get_permissions.side_effect = self.make_problem_reporting_side_effect(
            mock_get_permissions_problems,
            mock_get_permissions_return_values)

        resource_group_1 = mock.MagicMock()
        resource_group_2 = mock.MagicMock()
        deployment_access = mock.MagicMock()
        deployment_access.deployment.resource_groups = [ resource_group_1, resource_group_2 ]

        get_explicit_role_mappings_problem = mock.MagicMock()
        role_mappings = mock.MagicMock()
        mock_get_explicit_role_mappings.side_effect = self.make_problem_reporting_side_effect(
            [ get_explicit_role_mappings_problem ],
            [ role_mappings ])

        expected_problems = [ get_explicit_role_mappings_problem ]
        expected_problems.extend(mock_get_permissions_problems)

        problems = Custom_AccessControl.ProblemList()

        Custom_AccessControl._apply_deployment_access_control(request_type, deployment_access, problems)

        self.assertEquals(len(problems), len(expected_problems))

        mock_get_explicit_role_mappings.assert_called_with(deployment_access, problems)

        mock_get_resource_group_policy_name.assert_has_calls([
            mock.call(resource_group_1),
            mock.call(resource_group_2)])

        mock_get_permissions.assert_has_calls([
            mock.call(resource_group_1, problems),
            mock.call(resource_group_2, problems)])

        mock_update_roles.assert_not_called()


class UnitTest_CloudGemFramework_ProjectResourceHandler_Custom_AccessControl_apply_project_access_control(Custom_AccessControlTestCase):

    @mock.patch('resource_types.Custom_AccessControl._get_resource_group_policy_name')
    @mock.patch('resource_types.Custom_AccessControl._get_permissions')
    @mock.patch('resource_types.Custom_AccessControl._get_explicit_role_mappings')
    @mock.patch('resource_types.Custom_AccessControl._update_roles')
    def test_with_no_problems(self, 
            mock_update_roles,
            mock_get_explicit_role_mappings, 
            mock_get_permissions, 
            mock_get_resource_group_policy_name):

        request_type = mock.MagicMock()

        policy_name_a_1 = mock.MagicMock()
        policy_name_a_2 = mock.MagicMock()
        policy_name_b_1 = mock.MagicMock()
        policy_name_b_2 = mock.MagicMock()
        mock_get_resource_group_policy_name.side_effect = [ policy_name_a_1, policy_name_a_2, policy_name_b_1, policy_name_b_2 ]

        permissions_a_1 = mock.MagicMock()
        permissions_a_2 = mock.MagicMock()
        permissions_b_1 = mock.MagicMock()
        permissions_b_2 = mock.MagicMock()
        permissions_c_1 = mock.MagicMock()
        mock_get_permissions.side_effect = [ permissions_a_1, permissions_a_2, permissions_b_1, permissions_b_2, permissions_c_1 ]

        resource_group_a_1 = mock.MagicMock()
        resource_group_a_2 = mock.MagicMock()
        resource_group_b_1 = mock.MagicMock()
        resource_group_b_2 = mock.MagicMock()

        deployment_a = mock.MagicMock()
        deployment_a.resource_groups = [ resource_group_a_1, resource_group_a_2 ]

        deployment_b = mock.MagicMock()
        deployment_b.resource_groups = [ resource_group_b_1, resource_group_b_2 ]

        project = mock.MagicMock()
        project.deployments = [ deployment_a, deployment_b ]

        role_mappings = mock_get_explicit_role_mappings.return_value

        expected_problems = []

        problems = Custom_AccessControl.ProblemList()

        actual_problems = Custom_AccessControl._apply_project_access_control(request_type, project, problems)

        self.assertEquals(len(problems), len(expected_problems))

        mock_get_explicit_role_mappings.assert_called_with(project, problems)

        mock_get_resource_group_policy_name.assert_has_calls([
            mock.call(resource_group_a_1),
            mock.call(resource_group_a_2),
            mock.call(resource_group_b_1),
            mock.call(resource_group_b_2)])

        mock_get_permissions.assert_has_calls([
            mock.call(resource_group_a_1, problems),
            mock.call(resource_group_a_2, problems),
            mock.call(resource_group_b_1, problems),
            mock.call(resource_group_b_2, problems)])

        mock_update_roles.assert_has_calls([
            mock.call(request_type, policy_name_a_1, permissions_a_1, role_mappings),
            mock.call(request_type, policy_name_a_2, permissions_a_2, role_mappings),
            mock.call(request_type, policy_name_b_1, permissions_b_1, role_mappings),
            mock.call(request_type, policy_name_b_2, permissions_b_2, role_mappings)],
            any_order = True)


    @mock.patch('resource_types.Custom_AccessControl._get_resource_group_policy_name')
    @mock.patch('resource_types.Custom_AccessControl._get_permissions')
    @mock.patch('resource_types.Custom_AccessControl._get_explicit_role_mappings')
    @mock.patch('resource_types.Custom_AccessControl._update_roles')
    def test_with_problems(self, 
            mock_update_roles,
            mock_get_explicit_role_mappings, 
            mock_get_permissions, 
            mock_get_resource_group_policy_name):

        request_type = mock.MagicMock()

        policy_name_a_1 = mock.MagicMock()
        policy_name_a_2 = mock.MagicMock()
        policy_name_b_1 = mock.MagicMock()
        policy_name_b_2 = mock.MagicMock()
        mock_get_resource_group_policy_name.side_effect = [ policy_name_a_1, policy_name_a_2, policy_name_b_1, policy_name_b_2 ]

        permissions_a_1 = mock.MagicMock()
        permissions_a_2 = mock.MagicMock()
        permissions_b_1 = mock.MagicMock()
        permissions_b_2 = mock.MagicMock()
        permissions_c_1 = mock.MagicMock()
        permissions = [ permissions_a_1, permissions_a_2, permissions_b_1, permissions_b_2, permissions_c_1 ]

        mock_get_permissions_problem_a_1 = 'mock_get_permissions_problem_a_1'
        mock_get_permissions_problem_a_2 = 'mock_get_permissions_problem_a_2'
        mock_get_permissions_problem_b_1 = 'mock_get_permissions_problem_b_1'
        mock_get_permissions_problem_b_2 = 'mock_get_permissions_problem_b_2'
        mock_get_permissions_problem_c_1 = 'mock_get_permissions_problem_c_1'
        mock_get_permissions_problems = [ mock_get_permissions_problem_a_1, mock_get_permissions_problem_a_2, mock_get_permissions_problem_b_1, mock_get_permissions_problem_b_2, mock_get_permissions_problem_c_1 ]

        mock_get_permissions.side_effect = self.make_problem_reporting_side_effect(
            mock_get_permissions_problems,
            permissions)

        resource_group_a_1 = mock.MagicMock()
        resource_group_a_2 = mock.MagicMock()
        resource_group_b_1 = mock.MagicMock()
        resource_group_b_2 = mock.MagicMock()

        deployment_a = mock.MagicMock()
        deployment_a.resource_groups = [ resource_group_a_1, resource_group_a_2 ]

        deployment_b = mock.MagicMock()
        deployment_b.resource_groups = [ resource_group_b_1, resource_group_b_2 ]

        project = mock.MagicMock()
        project.deployments = [ deployment_a, deployment_b ]

        explicit_role_mappings = mock.MagicMock()
        mock_get_explicit_role_mappings_problem = 'mock_get_explicit_role_mappings_problem'
        mock_get_explicit_role_mappings.side_effect = self.make_problem_reporting_side_effect(
            [ mock_get_explicit_role_mappings_problem ],
            [ explicit_role_mappings ])

        actual_problems = Custom_AccessControl.ProblemList()

        Custom_AccessControl._apply_project_access_control(request_type, project, actual_problems)

        expected_problems = [ mock_get_explicit_role_mappings_problem ]
        expected_problems.extend( mock_get_permissions_problems )
        self.assertEquals(len(actual_problems), len(expected_problems))

        mock_get_explicit_role_mappings.assert_called_with(project, actual_problems)

        mock_get_resource_group_policy_name.assert_has_calls([
            mock.call(resource_group_a_1),
            mock.call(resource_group_a_2),
            mock.call(resource_group_b_1),
            mock.call(resource_group_b_2)])

        mock_get_permissions.assert_has_calls([
            mock.call(resource_group_a_1, actual_problems),
            mock.call(resource_group_a_2, actual_problems),
            mock.call(resource_group_b_1, actual_problems),
            mock.call(resource_group_b_2, actual_problems)])

        mock_update_roles.assert_not_called()


class UnitTest_CloudGemFramework_ProjectResourceHandler_Custom_AccessControl_get_resource_group_policy_name(Custom_AccessControlTestCase):

    def test_default(self):
        resource_group_name = 'test-resource-group'
        deployment_name = 'test-deployment'
        resource_group = mock.MagicMock()
        resource_group.resource_group_name = resource_group_name
        resource_group.deployment.deployment_name = deployment_name
        expected_policy_name = deployment_name + '.' + resource_group_name + '-AccessControl'
        actual_policy_name = Custom_AccessControl._get_resource_group_policy_name(resource_group)
        self.assertEquals(actual_policy_name, expected_policy_name)


class UnitTest_CloudGemFramework_ProjectResourceHandler_Custom_AccessControl_get_permissions(Custom_AccessControlTestCase):

    @mock.patch('resource_types.Custom_AccessControl._get_permission_list', return_value=[])
    def test_with_no_metadata(self, mock_get_permission_list):
        
        resource_a = mock.MagicMock()
        resource_a.get_cloud_canvas_metadata.return_value = None
        
        resource_b = mock.MagicMock()
        resource_b.get_cloud_canvas_metadata.return_value = None
        
        resource_group = mock.MagicMock()
        resource_group.resources = [ resource_a, resource_b ]
        
        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = []
        
        expected_permissions = {}
        
        actual_permissions = Custom_AccessControl._get_permissions(resource_group, actual_problems)
        
        self.assertEquals(actual_permissions, expected_permissions)
        self.assertEquals(len(actual_problems), len(expected_problems))
        
        resource_a.get_cloud_canvas_metadata.assert_called_with('Permissions')
        resource_b.get_cloud_canvas_metadata.assert_called_with('Permissions')


    @mock.patch('resource_types.Custom_AccessControl._get_permission_list')
    def test_with_metadata(self, 
            mock_get_permission_list):
        
        resource_a = mock.MagicMock(name='resource_a')
        metadata_a = resource_a.get_cloud_canvas_metadata.return_value
        
        resource_b = mock.MagicMock(name='resource_b')
        metadata_b = resource_b.get_cloud_canvas_metadata.return_value
        
        resource_group = mock.MagicMock(name='resource-group')
        resource_group.resources = [ resource_a, resource_b ]
        resource_group_default_role_mappings = resource_group.resource_definitions.get().permission_metadata.get()

        permission_list_a = [ mock.MagicMock(name='permission_list_a') ]
        permission_list_b = [ mock.MagicMock(name='permission_list_b') ]
        mock_get_permission_list.side_effect = [ permission_list_a, [], permission_list_b, [] ]

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = []

        expected_permissions = {
            resource_a.resource_arn: permission_list_a,
            resource_b.resource_arn: permission_list_b
        }
        
        actual_permissions = Custom_AccessControl._get_permissions(resource_group, actual_problems)

        self.assertEquals(actual_permissions, expected_permissions)
        self.assertEquals(len(actual_problems), len(expected_problems))

        resource_a.get_cloud_canvas_metadata.assert_called_with('Permissions')
        resource_b.get_cloud_canvas_metadata.assert_called_with('Permissions')
        
        mock_get_permission_list.assert_has_calls([
            mock.call(resource_group.permission_context_name, resource_a.logical_id, metadata_a, actual_problems),
            mock.call(resource_group.permission_context_name, resource_a.logical_id, resource_group_default_role_mappings, actual_problems),
            mock.call(resource_group.permission_context_name, resource_b.logical_id, metadata_b, actual_problems),
            mock.call(resource_group.permission_context_name, resource_b.logical_id, resource_group_default_role_mappings, actual_problems)])


    @mock.patch('resource_types.Custom_AccessControl._get_permission_list')
    def test_with_get_permission_list_problem(self, 
            mock_get_permission_list):
        
        resource_a = mock.MagicMock(name='resource_a')
        metadata_a = resource_a.get_cloud_canvas_metadata.return_value
        
        resource_b = mock.MagicMock(name='resource_b')
        metadata_b = resource_b.get_cloud_canvas_metadata.return_value
        
        resource_group = mock.MagicMock(name='resource-group')
        resource_group.resources = [ resource_a, resource_b ]
        resource_group_default_role_mappings = resource_group.resource_definitions.get().permission_metadata.get()

        problem_a_1 = 'problem_a_1'
        problem_a_2 = 'problem_a_2'
        problem_b_1 = 'problem_b_1'
        problem_b_2 = 'problem_b_2'

        mock_get_permission_list.side_effect = self.make_problem_reporting_side_effect(
            [ problem_a_1, problem_a_2, problem_b_1, problem_b_2 ],
            [ [], [], [], [] ])

        actual_problems = Custom_AccessControl.ProblemList()

        expected_problems = [ problem_a_1, problem_a_2, problem_b_1, problem_b_2 ]

        expected_permissions = {}
        
        actual_permissions = Custom_AccessControl._get_permissions(resource_group, actual_problems)

        self.assertEquals(actual_permissions, expected_permissions)
        self.assertEquals(len(actual_problems), len(expected_problems))

        resource_a.get_cloud_canvas_metadata.assert_called_with('Permissions')
        resource_b.get_cloud_canvas_metadata.assert_called_with('Permissions')
        
        mock_get_permission_list.assert_has_calls([
            mock.call(resource_group.permission_context_name, resource_a.logical_id, metadata_a, actual_problems),
            mock.call(resource_group.permission_context_name, resource_a.logical_id, resource_group_default_role_mappings, actual_problems),
            mock.call(resource_group.permission_context_name, resource_b.logical_id, metadata_b, actual_problems),
            mock.call(resource_group.permission_context_name, resource_b.logical_id, resource_group_default_role_mappings, actual_problems)])


    @mock.patch('resource_types.Custom_AccessControl._get_permission_list')
    def test_with_unsupported_resource_type_arn(self, 
            mock_get_permission_list):
        
        resource_a = mock.MagicMock(name='resource_a')
        type(resource_a).resource_arn = mock.PropertyMock(side_effect=RuntimeError('message_a'))
        metadata_a = resource_a.get_cloud_canvas_metadata.return_value
        
        resource_b = mock.MagicMock(name='resource_b')
        type(resource_b).resource_arn = mock.PropertyMock(side_effect=RuntimeError('message_b'))
        metadata_b = resource_b.get_cloud_canvas_metadata.return_value
        
        resource_group = mock.MagicMock(name='resource-group')
        resource_group.resources = [ resource_a, resource_b ]
        resource_group_default_role_mappings = resource_group.resource_definitions.get().permission_metadata.get()

        permission_list_a = [ mock.MagicMock(name='permission_list_a') ]
        permission_list_b = [ mock.MagicMock(name='permission_list_b') ]
        mock_get_permission_list.side_effect = [ permission_list_a, [], permission_list_b, [] ]

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = [ self.ANY_STRING, self.ANY_STRING ]

        expected_permissions = {}
        
        actual_permissions = Custom_AccessControl._get_permissions(resource_group, actual_problems)

        self.assertEquals(actual_permissions, expected_permissions)
        self.assertEquals(len(actual_problems), len(expected_problems))

        resource_a.get_cloud_canvas_metadata.assert_called_with('Permissions')
        resource_b.get_cloud_canvas_metadata.assert_called_with('Permissions')
        
        mock_get_permission_list.assert_has_calls([
            mock.call(resource_group.permission_context_name, resource_a.logical_id, metadata_a, actual_problems),
            mock.call(resource_group.permission_context_name, resource_a.logical_id, resource_group_default_role_mappings, actual_problems),
            mock.call(resource_group.permission_context_name, resource_b.logical_id, metadata_b, actual_problems),
            mock.call(resource_group.permission_context_name, resource_b.logical_id, resource_group_default_role_mappings, actual_problems)])


class UnitTest_CloudGemFramework_ProjectResourceHandler_Custom_AccessControl_get_permission_list(Custom_AccessControlTestCase):

    @mock.patch('resource_types.Custom_AccessControl._get_permission')
    def test_with_metadata_object(self, 
            mock_get_permission):
        
        permission = mock_get_permission.return_value

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = []

        expected_permission_list = [ permission ]
        
        resource_group_name = mock.MagicMock()
        resource_logical_id = mock.MagicMock()

        permission_metadata = mock.MagicMock()
        permission_metadata_list = permission_metadata

        actual_permission_list = Custom_AccessControl._get_permission_list(resource_group_name, resource_logical_id, permission_metadata_list, actual_problems)

        self.assertEquals(actual_permission_list, expected_permission_list)
        self.assertEquals(len(actual_problems), len(expected_problems))

        mock_get_permission.assert_has_calls([
            mock.call(resource_group_name, resource_logical_id, permission_metadata, actual_problems)])


    @mock.patch('resource_types.Custom_AccessControl._get_permission')
    def test_with_metadata_list(self, 
            mock_get_permission):

        permission_a = mock.MagicMock()        
        permission_b = mock.MagicMock()        
        mock_get_permission.side_effect = [ permission_a, permission_b ]

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = []

        expected_permission_list = [ permission_a, permission_b ]
        
        resource_group_name = mock.MagicMock()
        resource_logical_id = mock.MagicMock()

        permission_metadata_a = mock.MagicMock()
        permission_metadata_b = mock.MagicMock()
        permission_metadata_list = [ permission_metadata_a, permission_metadata_b ]

        actual_permission_list = Custom_AccessControl._get_permission_list(resource_group_name, resource_logical_id, permission_metadata_list, actual_problems)

        self.assertEquals(actual_permission_list, expected_permission_list)
        self.assertEquals(len(actual_problems), len(expected_problems))

        mock_get_permission.assert_has_calls([
            mock.call(resource_group_name, resource_logical_id, permission_metadata_a, actual_problems),
            mock.call(resource_group_name, resource_logical_id, permission_metadata_b, actual_problems)])


    @mock.patch('resource_types.Custom_AccessControl._get_permission')
    def test_with_get_permission_problem(self, 
            mock_get_permission):
        
        problem_a = mock.MagicMock()        
        problem_b = mock.MagicMock()        
        mock_get_permission.side_effect = self.make_problem_reporting_side_effect(
            [ problem_a, problem_b ],
            [ None, None ])

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = [ problem_a, problem_b ]

        expected_permission_list = []
        
        resource_group_name = mock.MagicMock()
        resource_logical_id = mock.MagicMock()

        permission_metadata_a = mock.MagicMock()
        permission_metadata_b = mock.MagicMock()
        permission_metadata_list = [ permission_metadata_a, permission_metadata_b ]

        actual_permission_list = Custom_AccessControl._get_permission_list(resource_group_name, resource_logical_id, permission_metadata_list, actual_problems)

        self.assertEquals(actual_permission_list, expected_permission_list)
        self.assertEquals(len(actual_problems), len(expected_problems))

        mock_get_permission.assert_has_calls([
            mock.call(resource_group_name, resource_logical_id, permission_metadata_a, actual_problems),
            mock.call(resource_group_name, resource_logical_id, permission_metadata_b, actual_problems)])


class UnitTest_CloudGemFramework_ProjectResourceHandler_Custom_AccessControl_get_permission(Custom_AccessControlTestCase):

    def test_with_invalid_metadata_type(self):

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = [ self.ANY_STRING ]

        resource_group_name = mock.MagicMock()
        resource_logical_id = mock.MagicMock()

        permission_metadata = 'invalid'

        expected_permission = None

        actual_permission = Custom_AccessControl._get_permission(resource_group_name, resource_logical_id, permission_metadata, actual_problems)

        self.assertEquals(actual_permission, expected_permission)
        self.assertEquals(len(actual_problems), len(expected_problems))


    @mock.patch('resource_types.Custom_AccessControl._get_permission_abstract_role_list')
    @mock.patch('resource_types.Custom_AccessControl._get_permission_allowed_action_list')
    @mock.patch('resource_types.Custom_AccessControl._get_permission_resource_suffix_list')
    def test_with_valid_metadata_object(self,
        mock_get_permission_resource_suffix_list,
        mock_get_permission_allowed_action_list,
        mock_get_permission_abstract_role_list):

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = []

        resource_name = 'test-resource-group'
        resource_logical_id = 'test-resource-logical-id'

        abstract_role = 'test-abstract-role'
        allowed_action = 'test-allowed-action'
        resource_suffix = 'test-resource-suffix'

        permission_metadata = {
            'AbstractRole': abstract_role,
            'Action': allowed_action,
            'ResourceSuffix': resource_suffix
        }

        expected_permission = {
            'AbstractRole': mock_get_permission_abstract_role_list.return_value,
            'Action': mock_get_permission_allowed_action_list.return_value,
            'ResourceSuffix': mock_get_permission_resource_suffix_list.return_value,
            'LogicalResourceId': resource_logical_id
        }

        actual_permission = Custom_AccessControl._get_permission(resource_name, resource_logical_id, permission_metadata, actual_problems)

        self.assertEquals(actual_permission, expected_permission)
        self.assertEquals(len(actual_problems), len(expected_problems))

        mock_get_permission_resource_suffix_list.assert_has_calls([
            mock.call(resource_suffix, actual_problems)])

        mock_get_permission_allowed_action_list.assert_has_calls([
            mock.call(allowed_action, actual_problems)])

        mock_get_permission_abstract_role_list.assert_has_calls([
            mock.call(resource_name, abstract_role, actual_problems)])


    @mock.patch('resource_types.Custom_AccessControl._get_permission_abstract_role_list')
    @mock.patch('resource_types.Custom_AccessControl._get_permission_allowed_action_list')
    @mock.patch('resource_types.Custom_AccessControl._get_permission_resource_suffix_list')
    def test_with_metadata_object_with_unsupported_property(self,
        mock_get_permission_resource_suffix_list,
        mock_get_permission_allowed_action_list,
        mock_get_permission_abstract_role_list):

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = [ self.ANY_STRING ]

        resource_group_name = 'test-resource-group'
        resource_logical_id = 'test-resource-logical-id'

        abstract_role = 'test-abstract-role'
        allowed_action = 'test-allowed-action'
        resource_suffix = 'test-resource-suffix'

        permission_metadata = {
            'AbstractRole': abstract_role,
            'Action': allowed_action,
            'ResourceSuffix': resource_suffix,
            'Unsupported': 'unsupported-value'
        }

        expected_permission = {
            'AbstractRole': mock_get_permission_abstract_role_list.return_value,
            'Action': mock_get_permission_allowed_action_list.return_value,
            'ResourceSuffix': mock_get_permission_resource_suffix_list.return_value,
            'LogicalResourceId': resource_logical_id
        }

        actual_permission = Custom_AccessControl._get_permission(resource_group_name, resource_logical_id, permission_metadata, actual_problems)

        self.assertEquals(actual_permission, expected_permission)
        self.assertEquals(len(actual_problems), len(expected_problems))

        mock_get_permission_resource_suffix_list.assert_has_calls([
            mock.call(resource_suffix, actual_problems)])

        mock_get_permission_allowed_action_list.assert_has_calls([
            mock.call(allowed_action, actual_problems)])

        mock_get_permission_abstract_role_list.assert_has_calls([
            mock.call(resource_group_name, abstract_role, actual_problems)])


class UnitTest_CloudGemFramework_ProjectResourceHandler_Custom_AccessControl_get_permission_abstract_role_list(Custom_AccessControlTestCase):

    def test_with_none(self):

        resource_group_name = mock.MagicMock()

        abstract_role_list = None

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = [ self.ANY_STRING ]

        expected_abstract_role_list = []
        actual_abstract_role_list = Custom_AccessControl._get_permission_abstract_role_list(resource_group_name, abstract_role_list, actual_problems)
        
        self.assertEquals(actual_abstract_role_list, expected_abstract_role_list)
        self.assertEquals(len(actual_problems), len(expected_problems))


    def test_with_string(self):

        resource_group_name = mock.MagicMock()

        abstract_role = 'test-abstract-role'
        abstract_role_list = abstract_role

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = []

        expected_abstract_role_list = [ [ resource_group_name, abstract_role ] ]
        actual_abstract_role_list = Custom_AccessControl._get_permission_abstract_role_list(resource_group_name, abstract_role_list, actual_problems)
        
        self.assertEquals(actual_abstract_role_list, expected_abstract_role_list)
        self.assertEquals(len(actual_problems), len(expected_problems))


    def test_with_list(self):

        resource_group_name = mock.MagicMock()

        abstract_role_1 = 'test-abstract-role-1'
        abstract_role_2 = 'test-abstract-role-2'
        abstract_role_list = [ abstract_role_1, abstract_role_2 ]

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = []

        expected_abstract_role_list = [ 
            [ resource_group_name, abstract_role_1 ],
            [ resource_group_name, abstract_role_2 ]
        ]

        actual_abstract_role_list = Custom_AccessControl._get_permission_abstract_role_list(resource_group_name, abstract_role_list, actual_problems)
        
        self.assertEquals(actual_abstract_role_list, expected_abstract_role_list)
        self.assertEquals(len(actual_problems), len(expected_problems))


    def test_with_object(self):

        resource_group_name = mock.MagicMock()

        abstract_role_list = {}

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = [ self.ANY_STRING ]

        expected_abstract_role_list = []

        actual_abstract_role_list = Custom_AccessControl._get_permission_abstract_role_list(resource_group_name, abstract_role_list, actual_problems)
        
        self.assertEquals(actual_abstract_role_list, expected_abstract_role_list)
        self.assertEquals(len(actual_problems), len(expected_problems))


    def test_with_string_with_dot(self):

        resource_group_name = mock.MagicMock()

        abstract_role = 'test-abstract-role.with-dot'
        abstract_role_list = abstract_role

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = [ self.ANY_STRING ]

        expected_abstract_role_list = []
        actual_abstract_role_list = Custom_AccessControl._get_permission_abstract_role_list(resource_group_name, abstract_role_list, actual_problems)
        
        self.assertEquals(actual_abstract_role_list, expected_abstract_role_list)
        self.assertEquals(len(actual_problems), len(expected_problems))


class UnitTest_CloudGemFramework_ProjectResourceHandler_Custom_AccessControl_get_permission_resource_suffix_list(Custom_AccessControlTestCase):

    def test_with_none(self):

        resource_suffix_list = None

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = []

        expected_resource_suffix_list = ['']
        actual_resource_suffix_list = Custom_AccessControl._get_permission_resource_suffix_list(resource_suffix_list, actual_problems)
        
        self.assertEquals(actual_resource_suffix_list, expected_resource_suffix_list)
        self.assertEquals(len(actual_problems), len(expected_problems))


    def test_with_string(self):

        resource_suffix = 'test-resource-suffix'
        resource_suffix_list = resource_suffix

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = []

        expected_resource_suffix_list = [ resource_suffix ]
        actual_resource_suffix_list = Custom_AccessControl._get_permission_resource_suffix_list(resource_suffix_list, actual_problems)
        
        self.assertEquals(actual_resource_suffix_list, expected_resource_suffix_list)
        self.assertEquals(len(actual_problems), len(expected_problems))


    def test_with_list(self):

        resource_suffix_1 = 'test-resource-suffix-1'
        resource_suffix_2 = 'test-resource-suffix-2'
        resource_suffix_list = [ resource_suffix_1, resource_suffix_2 ]

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = []

        expected_resource_suffix_list = resource_suffix_list

        actual_resource_suffix_list = Custom_AccessControl._get_permission_resource_suffix_list(resource_suffix_list, actual_problems)
        
        self.assertEquals(actual_resource_suffix_list, expected_resource_suffix_list)
        self.assertEquals(len(actual_problems), len(expected_problems))


    def test_with_empty_list(self):

        resource_suffix_list = []

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = []

        expected_resource_suffix_list = ['']

        actual_resource_suffix_list = Custom_AccessControl._get_permission_resource_suffix_list(resource_suffix_list, actual_problems)
        
        self.assertEquals(actual_resource_suffix_list, expected_resource_suffix_list)
        self.assertEquals(len(actual_problems), len(expected_problems))


    def test_with_object(self):

        resource_suffix_list = {}

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = [ self.ANY_STRING ]

        expected_resource_suffix_list = []

        actual_resource_suffix_list = Custom_AccessControl._get_permission_resource_suffix_list(resource_suffix_list, actual_problems)
        
        self.assertEquals(actual_resource_suffix_list, expected_resource_suffix_list)
        self.assertEquals(len(actual_problems), len(expected_problems))


class UnitTest_CloudGemFramework_ProjectResourceHandler_Custom_AccessControl_get_permission_allowed_action_list(Custom_AccessControlTestCase):

    def test_with_none(self):

        allowed_action_list = None

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = [ self.ANY_STRING ]

        expected_allowed_action_list = []
        actual_allowed_action_list = Custom_AccessControl._get_permission_allowed_action_list(allowed_action_list, actual_problems)
        
        self.assertEquals(actual_allowed_action_list, expected_allowed_action_list)
        self.assertEquals(len(actual_problems), len(expected_problems))


    def test_with_string(self):

        allowed_action = 'test-allowed-action'
        allowed_action_list = allowed_action

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = []

        expected_allowed_action_list = [ allowed_action ]
        actual_allowed_action_list = Custom_AccessControl._get_permission_allowed_action_list(allowed_action_list, actual_problems)
        
        self.assertEquals(actual_allowed_action_list, expected_allowed_action_list)
        self.assertEquals(len(actual_problems), len(expected_problems))


    def test_with_list(self):

        allowed_action_1 = 'test-allowed-action-1'
        allowed_action_2 = 'test-allowed-action-2'
        allowed_action_list = [ allowed_action_1, allowed_action_2 ]

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = []

        expected_allowed_action_list = allowed_action_list

        actual_allowed_action_list = Custom_AccessControl._get_permission_allowed_action_list(allowed_action_list, actual_problems)
        
        self.assertEquals(actual_allowed_action_list, expected_allowed_action_list)
        self.assertEquals(len(actual_problems), len(expected_problems))


    def test_with_object(self):

        allowed_action_list = {}

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = [ self.ANY_STRING ]

        expected_allowed_action_list = []

        actual_allowed_action_list = Custom_AccessControl._get_permission_allowed_action_list(allowed_action_list, actual_problems)
        
        self.assertEquals(actual_allowed_action_list, expected_allowed_action_list)
        self.assertEquals(len(actual_problems), len(expected_problems))


class UnitTest_CloudGemFramework_ProjectResourceHandler_Custom_AccessControl_get_implicit_role_mappings(Custom_AccessControlTestCase):

    def test_default(self):

        mock_physical_role_name_1 = 'physical-test-role-1'
        mock_logical_role_name_1 = 'logical-test-role-1'
        mock_resource_1_id_data = { 'AbstractRoleMappings': { mock_logical_role_name_1: mock_physical_role_name_1 } }
        mock_resource_1 = mock.MagicMock()
        mock_resource_1.type = 'Custom::Test'
        mock_resource_1.physical_id = 'resource-name::{}'.format(json.dumps(mock_resource_1_id_data))

        mock_physical_role_name_2 = 'physical-test-role-2'
        mock_logical_role_name_2 = 'logical-test-role-2'
        mock_resource_2_id_data = { 'AbstractRoleMappings': { mock_logical_role_name_2: mock_physical_role_name_2 } }
        mock_resource_2 = mock.MagicMock()
        mock_resource_2.type = 'Custom::Test'
        mock_resource_2.physical_id = 'resource-name::{}'.format(json.dumps(mock_resource_2_id_data))

        mock_resource_info_name = 'test-resource-group'

        mock_resource_info = mock.MagicMock()
        mock_resource_info.permission_context_name = mock_resource_info_name
        mock_resource_info.resources = [ mock_resource_1, mock_resource_2 ]

        problems = []

        expected_mappings = {
            mock_physical_role_name_1: [{
                'Effect': 'Allow',
                'AbstractRole': [ [ mock_resource_info_name, mock_logical_role_name_1.encode('utf8') ] ]
            }],
            mock_physical_role_name_2: [{
                'Effect': 'Allow',
                'AbstractRole': [ [ mock_resource_info_name, mock_logical_role_name_2.encode('utf8') ] ]
            }]
        }

        actual_mappings = Custom_AccessControl._get_implicit_role_mappings(mock_resource_info, problems)

        self.assertEquals(actual_mappings, expected_mappings)


class UnitTest_CloudGemFramework_ProjectResourceHandler_Custom_AccessControl_get_explicit_role_mappings(Custom_AccessControlTestCase):

    def test_with_no_metadata(self):
        
        role_resource_a = mock.MagicMock()
        role_resource_a.get_cloud_canvas_metadata.return_value = None
        
        role_resource_b = mock.MagicMock()
        role_resource_b.get_cloud_canvas_metadata.return_value = None
        
        stack = mock.MagicMock()
        stack.resources.get_by_type.return_value = [ role_resource_a, role_resource_b ]
        
        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = []
        
        expected_role_mappings = {
            role_resource_a.physical_id: [],
            role_resource_b.physical_id: []
        }
        
        actual_role_mappings = Custom_AccessControl._get_explicit_role_mappings(stack, actual_problems)
        
        self.assertEquals(actual_role_mappings, expected_role_mappings)
        self.assertEquals(len(actual_problems), len(expected_problems))
        
        role_resource_a.get_cloud_canvas_metadata.assert_called_with('RoleMappings')
        role_resource_b.get_cloud_canvas_metadata.assert_called_with('RoleMappings')

        stack.resources.get_by_type.assert_called_with('AWS::IAM::Role')


    @mock.patch('resource_types.Custom_AccessControl._get_role_mapping_list')
    def test_with_metadata(self, 
            mock_get_role_mapping_list):
        
        role_resource_a = mock.MagicMock(name='resource_a')
        metadata_a = role_resource_a.get_cloud_canvas_metadata.return_value
        
        role_resource_b = mock.MagicMock(name='resource_b')
        metadata_b = role_resource_b.get_cloud_canvas_metadata.return_value
        
        stack = mock.MagicMock(name='resource-group')
        stack.resources.get_by_type.return_value = [ role_resource_a, role_resource_b ]

        role_mapping_list_a = mock.MagicMock(name='role_mapping_list_a')
        role_mapping_list_b = mock.MagicMock(name='role_mapping_list_b')
        mock_get_role_mapping_list.side_effect = [ role_mapping_list_a, role_mapping_list_b ]

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = []

        expected_role_mappings = {
            role_resource_a.physical_id: role_mapping_list_a,
            role_resource_b.physical_id: role_mapping_list_b
        }
        
        actual_role_mappings = Custom_AccessControl._get_explicit_role_mappings(stack, actual_problems)

        self.assertEquals(actual_role_mappings, expected_role_mappings)
        self.assertEquals(len(actual_problems), len(expected_problems))

        role_resource_a.get_cloud_canvas_metadata.assert_called_with('RoleMappings')
        role_resource_b.get_cloud_canvas_metadata.assert_called_with('RoleMappings')
        
        stack.resources.get_by_type.assert_called_with('AWS::IAM::Role')

        mock_get_role_mapping_list.assert_has_calls([
            mock.call(metadata_a, actual_problems),
            mock.call(metadata_b, actual_problems)])


    @mock.patch('resource_types.Custom_AccessControl._get_role_mapping_list')
    def test_with_get_role_mapping_list_problem(self, 
            mock_get_role_mapping_list):
        
        role_resource_a = mock.MagicMock(name='resource_a')
        metadata_a = role_resource_a.get_cloud_canvas_metadata.return_value
        
        role_resource_b = mock.MagicMock(name='resource_b')
        metadata_b = role_resource_b.get_cloud_canvas_metadata.return_value
        
        stack = mock.MagicMock(name='resource-group')
        stack.resources.get_by_type.return_value = [ role_resource_a, role_resource_b ]

        problem_a_1 = mock.MagicMock(name='problem_a_1')
        problem_b_1 = mock.MagicMock(name='problem_b_1')

        mock_get_role_mapping_list.side_effect = self.make_problem_reporting_side_effect(
            [ problem_a_1, problem_b_1 ],
            [ [], [] ])

        actual_problems = Custom_AccessControl.ProblemList()

        expected_problems = [ problem_a_1, problem_b_1 ]

        expected_role_mappings = {
            role_resource_a.physical_id: [],
            role_resource_b.physical_id: []
        }

        actual_role_mappings = Custom_AccessControl._get_explicit_role_mappings(stack, actual_problems)

        self.assertEquals(actual_role_mappings, expected_role_mappings)
        self.assertEquals(len(actual_problems), len(expected_problems))

        role_resource_a.get_cloud_canvas_metadata.assert_called_with('RoleMappings')
        role_resource_b.get_cloud_canvas_metadata.assert_called_with('RoleMappings')
        
        stack.resources.get_by_type.assert_called_with('AWS::IAM::Role')

        mock_get_role_mapping_list.assert_has_calls([
            mock.call(metadata_a, actual_problems),
            mock.call(metadata_b, actual_problems)])


class UnitTest_CloudGemFramework_ProjectResourceHandler_Custom_AccessControl_get_role_mapping_list(Custom_AccessControlTestCase):

    @mock.patch('resource_types.Custom_AccessControl._get_role_mapping')
    def test_with_metadata_object(self, 
            mock_get_role_mapping):
        
        role_mapping = mock_get_role_mapping.return_value

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = []

        expected_role_mapping_list = [ role_mapping ]
        
        role_mapping_metadata = mock.MagicMock()
        role_mapping_metadata_list = role_mapping_metadata

        actual_role_mapping_list = Custom_AccessControl._get_role_mapping_list(role_mapping_metadata_list, actual_problems)

        self.assertEquals(actual_role_mapping_list, expected_role_mapping_list)
        self.assertEquals(len(actual_problems), len(expected_problems))

        mock_get_role_mapping.assert_has_calls([
            mock.call(role_mapping_metadata, actual_problems)])


    @mock.patch('resource_types.Custom_AccessControl._get_role_mapping')
    def test_with_metadata_list(self, 
            mock_get_role_mapping):

        role_mapping_a = mock.MagicMock()        
        role_mapping_b = mock.MagicMock()        
        mock_get_role_mapping.side_effect = [ role_mapping_a, role_mapping_b ]

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = []

        expected_role_mapping_list = [ role_mapping_a, role_mapping_b ]
        
        role_mapping_metadata_a = mock.MagicMock()
        role_mapping_metadata_b = mock.MagicMock()
        role_mapping_metadata_list = [ role_mapping_metadata_a, role_mapping_metadata_b ]

        actual_role_mapping_list = Custom_AccessControl._get_role_mapping_list(role_mapping_metadata_list, actual_problems)

        self.assertEquals(actual_role_mapping_list, expected_role_mapping_list)
        self.assertEquals(len(actual_problems), len(expected_problems))

        mock_get_role_mapping.assert_has_calls([
            mock.call(role_mapping_metadata_a, actual_problems),
            mock.call(role_mapping_metadata_b, actual_problems)])


    @mock.patch('resource_types.Custom_AccessControl._get_role_mapping')
    def test_with_get_role_mapping_problem(self, 
            mock_get_role_mapping):
        
        problem_a = mock.MagicMock()        
        problem_b = mock.MagicMock()        
        mock_get_role_mapping.side_effect = self.make_problem_reporting_side_effect(
            [ problem_a, problem_b ],
            [ None, None ])

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = [ problem_a, problem_b ]

        expected_role_mapping_list = []
        
        role_mapping_metadata_a = mock.MagicMock()
        role_mapping_metadata_b = mock.MagicMock()
        role_mapping_metadata_list = [ role_mapping_metadata_a, role_mapping_metadata_b ]

        actual_role_mapping_list = Custom_AccessControl._get_role_mapping_list(role_mapping_metadata_list, actual_problems)

        self.assertEquals(actual_role_mapping_list, expected_role_mapping_list)
        self.assertEquals(len(actual_problems), len(expected_problems))

        mock_get_role_mapping.assert_has_calls([
            mock.call(role_mapping_metadata_a, actual_problems),
            mock.call(role_mapping_metadata_b, actual_problems)])


class UnitTest_CloudGemFramework_ProjectResourceHandler_Custom_AccessControl_get_role_mapping(Custom_AccessControlTestCase):

    def test_with_invalid_metadata_type(self):

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = [ self.ANY_STRING ]

        role_mapping_metadata = 'invalid'

        expected_role_mapping = None

        actual_role_mapping = Custom_AccessControl._get_role_mapping(role_mapping_metadata, actual_problems)

        self.assertEquals(actual_role_mapping, expected_role_mapping)
        self.assertEquals(len(actual_problems), len(expected_problems))


    @mock.patch('resource_types.Custom_AccessControl._get_role_mapping_abstract_role_list')
    @mock.patch('resource_types.Custom_AccessControl._get_role_mapping_effect')
    def test_with_valid_metadata_object(self,
        mock_get_role_mapping_effect,
        mock_get_role_mapping_abstract_role_list):

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = []

        abstract_role = 'test-abstract-role'
        effect = 'test-effect'

        role_mapping_metadata = {
            'AbstractRole': abstract_role,
            'Effect': effect
        }

        expected_role_mapping = {
            'AbstractRole': mock_get_role_mapping_abstract_role_list.return_value,
            'Effect': mock_get_role_mapping_effect.return_value
        }

        actual_role_mapping = Custom_AccessControl._get_role_mapping(role_mapping_metadata, actual_problems)

        self.assertEquals(actual_role_mapping, expected_role_mapping)
        self.assertEquals(len(actual_problems), len(expected_problems))

        mock_get_role_mapping_effect.assert_has_calls([
            mock.call(effect, actual_problems)])

        mock_get_role_mapping_abstract_role_list.assert_has_calls([
            mock.call(abstract_role, actual_problems)])


    @mock.patch('resource_types.Custom_AccessControl._get_role_mapping_abstract_role_list')
    @mock.patch('resource_types.Custom_AccessControl._get_role_mapping_effect')
    def test_with_metadata_object_with_unsupported_property(self,
        mock_get_role_mapping_effect,
        mock_get_role_mapping_abstract_role_list):

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = [ self.ANY_STRING ]

        abstract_role = 'test-abstract-role'
        effect = 'test-effect'

        role_mapping_metadata = {
            'AbstractRole': abstract_role,
            'Effect': effect,
            'Unsupported': 'unsupported-value'
        }

        expected_role_mapping = {
            'AbstractRole': mock_get_role_mapping_abstract_role_list.return_value,
            'Effect': mock_get_role_mapping_effect.return_value
        }

        actual_role_mapping = Custom_AccessControl._get_role_mapping(role_mapping_metadata, actual_problems)

        self.assertEquals(actual_role_mapping, expected_role_mapping)
        self.assertEquals(len(actual_problems), len(expected_problems))

        mock_get_role_mapping_effect.assert_has_calls([
            mock.call(effect, actual_problems)])

        mock_get_role_mapping_abstract_role_list.assert_has_calls([
            mock.call(abstract_role, actual_problems)])


class UnitTest_CloudGemFramework_ProjectResourceHandler_Custom_AccessControl_get_role_mapping_abstract_role_list(Custom_AccessControlTestCase):

    def test_with_none(self):

        abstract_role_list = None

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = [ self.ANY_STRING ]

        expected_abstract_role_list = []
        actual_abstract_role_list = Custom_AccessControl._get_role_mapping_abstract_role_list(abstract_role_list, actual_problems)
        
        self.assertEquals(actual_abstract_role_list, expected_abstract_role_list)
        self.assertEquals(len(actual_problems), len(expected_problems))


    def test_with_string(self):

        resource_group_name = 'test-resource-group'
        abstract_role_name = 'test-abstract-role'
        abstract_role = resource_group_name + '.' + abstract_role_name
        abstract_role_list = abstract_role

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = []

        expected_abstract_role_list = [ [ resource_group_name, abstract_role_name ] ]
        actual_abstract_role_list = Custom_AccessControl._get_role_mapping_abstract_role_list(abstract_role_list, actual_problems)
        
        self.assertEquals(actual_abstract_role_list, expected_abstract_role_list)
        self.assertEquals(len(actual_problems), len(expected_problems))


    def test_with_list(self):

        resource_group_name_1 = 'test-resource-group-1'
        resource_group_name_2 = 'test-resource-group-2'
        abstract_role_name_1 = 'test-abstract-role-1'
        abstract_role_name_2 = 'test-abstract-role-2'
        abstract_role_1 = resource_group_name_1 + '.' + abstract_role_name_1
        abstract_role_2 = resource_group_name_2 + '.' + abstract_role_name_2
        abstract_role_list = [ abstract_role_1, abstract_role_2 ]

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = []

        expected_abstract_role_list = [ 
            [ resource_group_name_1, abstract_role_name_1 ],
            [ resource_group_name_2, abstract_role_name_2 ]
        ]

        actual_abstract_role_list = Custom_AccessControl._get_role_mapping_abstract_role_list(abstract_role_list, actual_problems)
        
        self.assertEquals(actual_abstract_role_list, expected_abstract_role_list)
        self.assertEquals(len(actual_problems), len(expected_problems))


    def test_with_object(self):

        abstract_role_list = {}

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = [ self.ANY_STRING ]

        expected_abstract_role_list = []

        actual_abstract_role_list = Custom_AccessControl._get_role_mapping_abstract_role_list(abstract_role_list, actual_problems)
        
        self.assertEquals(actual_abstract_role_list, expected_abstract_role_list)
        self.assertEquals(len(actual_problems), len(expected_problems))


    def test_with_string_without_dot(self):

        abstract_role = 'invalid'
        abstract_role_list = abstract_role

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = [ self.ANY_STRING ]

        expected_abstract_role_list = []
        actual_abstract_role_list = Custom_AccessControl._get_role_mapping_abstract_role_list(abstract_role_list, actual_problems)
        
        self.assertEquals(actual_abstract_role_list, expected_abstract_role_list)
        self.assertEquals(len(actual_problems), len(expected_problems))


    def test_with_string_with_too_many_dots(self):

        abstract_role = 'invalid.with.dots'
        abstract_role_list = abstract_role

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = [ self.ANY_STRING ]

        expected_abstract_role_list = []
        actual_abstract_role_list = Custom_AccessControl._get_role_mapping_abstract_role_list(abstract_role_list, actual_problems)
        
        self.assertEquals(actual_abstract_role_list, expected_abstract_role_list)
        self.assertEquals(len(actual_problems), len(expected_problems))


class UnitTest_CloudGemFramework_ProjectResourceHandler_Custom_AccessControl_get_role_mapping_effect(Custom_AccessControlTestCase):

    def test_with_none(self):

        effect = None

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = [ self.ANY_STRING ]

        expected_effect = effect
        actual_effect = Custom_AccessControl._get_role_mapping_effect(effect, actual_problems)
        
        self.assertEquals(actual_effect, expected_effect)
        self.assertEquals(len(actual_problems), len(expected_problems))


    def test_with_Allow(self):

        effect = 'Allow'

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = []

        expected_effect = effect
        actual_effect = Custom_AccessControl._get_role_mapping_effect(effect, actual_problems)
        
        self.assertEquals(actual_effect, expected_effect)
        self.assertEquals(len(actual_problems), len(expected_problems))


    def test_with_Deny(self):

        effect = 'Deny'

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = []

        expected_effect = effect
        actual_effect = Custom_AccessControl._get_role_mapping_effect(effect, actual_problems)
        
        self.assertEquals(actual_effect, expected_effect)
        self.assertEquals(len(actual_problems), len(expected_problems))


    def test_with_invalid_type(self):

        effect = []

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = [ self.ANY_STRING ]

        expected_effect = effect

        actual_effect = Custom_AccessControl._get_role_mapping_effect(effect, actual_problems)
        
        self.assertEquals(actual_effect, expected_effect)
        self.assertEquals(len(actual_problems), len(expected_problems))


    def test_with_invalid_string(self):

        effect = 'Invalid'

        actual_problems = Custom_AccessControl.ProblemList()
        expected_problems = [ self.ANY_STRING ]

        expected_effect = effect

        actual_effect = Custom_AccessControl._get_role_mapping_effect(effect, actual_problems)
        
        self.assertEquals(actual_effect, expected_effect)
        self.assertEquals(len(actual_problems), len(expected_problems))


class UnitTest_CloudGemFramework_ProjectResourceHandler_Custom_AccessControl_update_roles(Custom_AccessControlTestCase):

    policy_name = 'TestPolicy'
    permissions = mock.MagicMock()
    mock_resource_group_name = 'test-resource-group'

    mock_physical_role_name_1 = 'physical-test-role-1'
    mock_physical_role_name_2 = 'physical-test-role-2'

    role_mapping_list_1 = mock.MagicMock()
    role_mapping_list_2 = mock.MagicMock()

    role_mappings = {
        mock_physical_role_name_1: role_mapping_list_1,
        mock_physical_role_name_2: role_mapping_list_2
    }

    @mock_aws.patch_client('iam', 'delete_role_policy', reload = Custom_AccessControl)
    def test_with_request_type_delete(self, mock_delete_role_policy):

        request_type = 'Delete'
        Custom_AccessControl._update_roles(request_type, self.policy_name, self.permissions, self.role_mappings)

        mock_delete_role_policy.assert_has_calls([
            mock.call(RoleName=self.mock_physical_role_name_1, PolicyName=self.policy_name),
            mock.call(RoleName=self.mock_physical_role_name_2, PolicyName=self.policy_name)], 
            any_order = True)


    @mock_aws.patch_client('iam', 'delete_role_policy', reload = Custom_AccessControl)
    @mock.patch('resource_types.Custom_AccessControl._create_role_policy')
    def test_with_no_statements(self, mock_create_role_policy, mock_delete_role_policy):

        mock_create_role_policy.return_value = { 'Statement': [] }

        request_type = 'Create'
        Custom_AccessControl._update_roles(request_type, self.policy_name, self.permissions, self.role_mappings)

        mock_create_role_policy.assert_has_calls([
            mock.call(self.permissions, self.role_mapping_list_1),
            mock.call(self.permissions, self.role_mapping_list_2)],
            any_order = True)

        mock_delete_role_policy.assert_has_calls([
            mock.call(RoleName=self.mock_physical_role_name_1, PolicyName=self.policy_name),
            mock.call(RoleName=self.mock_physical_role_name_2, PolicyName=self.policy_name)], 
            any_order = True)


    @mock_aws.patch_client('iam', 'put_role_policy', reload = Custom_AccessControl)
    @mock.patch('resource_types.Custom_AccessControl._create_role_policy')
    def test_with_statements(self, mock_create_role_policy, mock_put_role_policy):

        mock_policy_1 = { 'Statement': [ 'Policy-1' ] }
        mock_policy_2 = { 'Statement': [ 'Policy-2' ] }

        def mock_create_role_policy_side_effect(permissions, role_mapping_list):
            if role_mapping_list is self.role_mapping_list_1:
                return mock_policy_1
            elif role_mapping_list is self.role_mapping_list_2:
                return mock_policy_2
            else:
                return None

        mock_create_role_policy.side_effect = mock_create_role_policy_side_effect

        request_type = 'Create'
        Custom_AccessControl._update_roles(request_type, self.policy_name, self.permissions, self.role_mappings)

        mock_create_role_policy.assert_has_calls([
            mock.call(self.permissions, self.role_mapping_list_1),
            mock.call(self.permissions, self.role_mapping_list_2)],
            any_order = True)

        mock_put_role_policy.assert_has_calls([
            mock.call(RoleName=self.mock_physical_role_name_1, PolicyName=self.policy_name, PolicyDocument=json.dumps(mock_policy_1)),
            mock.call(RoleName=self.mock_physical_role_name_2, PolicyName=self.policy_name, PolicyDocument=json.dumps(mock_policy_2))], 
            any_order = True)


class UnitTest_CloudGemFramework_ProjectResourceHandler_Custom_AccessControl_create_role_policy(Custom_AccessControlTestCase):

    PERMISSION_RESOURCE_ARN_1 = 'permission-resource-arn-1'
    PERMISSION_RESOURCE_ARN_2 = 'permission-resource-arn-2'
    PERMISSION_ABSTRACT_ROLE_LIST_1_A = [ 'permission-abstract-role-list-1-a' ]
    PERMISSION_ABSTRACT_ROLE_LIST_1_B = [ 'permission-abstract-role-list-1-b' ]
    PERMISSION_ABSTRACT_ROLE_LIST_2_A = [ 'permission-abstract-role-list-2-a' ]
    PERMISSION_ABSTRACT_ROLE_LIST_2_B = [ 'permission-abstract-role-list-2-b' ]
    PERMISSION_ACTION_LIST_1_A = [ 'permission-action-list-1-a' ]
    PERMISSION_ACTION_LIST_1_B = [ 'permission-action-list-1-b' ]
    PERMISSION_ACTION_LIST_2_A = [ 'permission-action-list-2-a' ]
    PERMISSION_ACTION_LIST_2_B = [ 'permission-action-list-2-b' ]
    PERMISSION_RESOURCE_SUFFIX_1_A_P = 'permission-resource-suffix-1-a-p'
    PERMISSION_RESOURCE_SUFFIX_1_A_Q = 'permission-resource-suffix-1-a-q'
    PERMISSION_RESOURCE_SUFFIX_1_B_P = 'permission-resource-suffix-1-b-p'
    PERMISSION_RESOURCE_SUFFIX_1_B_Q = 'permission-resource-suffix-1-b-q'
    PERMISSION_RESOURCE_SUFFIX_2_A_P = 'permission-resource-suffix-2-a-p'
    PERMISSION_RESOURCE_SUFFIX_2_A_Q = 'permission-resource-suffix-2-a-q'
    PERMISSION_RESOURCE_SUFFIX_2_B_P = 'permission-resource-suffix-2-b-p'
    PERMISSION_RESOURCE_SUFFIX_2_B_Q = 'permission-resource-suffix-2-b-q'
    PERMISSION_RESOURCE_SUFFIX_LIST_1_A = [ PERMISSION_RESOURCE_SUFFIX_1_A_P, PERMISSION_RESOURCE_SUFFIX_1_A_Q ]
    PERMISSION_RESOURCE_SUFFIX_LIST_1_B = [ PERMISSION_RESOURCE_SUFFIX_1_B_P, PERMISSION_RESOURCE_SUFFIX_1_B_Q ]
    PERMISSION_RESOURCE_SUFFIX_LIST_2_A = [ PERMISSION_RESOURCE_SUFFIX_2_A_P, PERMISSION_RESOURCE_SUFFIX_2_A_Q ]
    PERMISSION_RESOURCE_SUFFIX_LIST_2_B = [ PERMISSION_RESOURCE_SUFFIX_2_B_P, PERMISSION_RESOURCE_SUFFIX_2_B_Q ]
    PERMISSION_SID_1_A = 'permission-sid-1-a'
    PERMISSION_SID_1_B = 'permission-sid-1-b'
    PERMISSION_SID_2_A = 'permission-sid-2-a'
    PERMISSION_SID_2_B = 'permission-sid-2-b'

    PERMISSIONS = {
        PERMISSION_RESOURCE_ARN_1: [
            {
                "AbstractRole": PERMISSION_ABSTRACT_ROLE_LIST_1_A,
                "Action": PERMISSION_ACTION_LIST_1_A,
                "ResourceSuffix": PERMISSION_RESOURCE_SUFFIX_LIST_1_A,
                "LogicalResourceId": PERMISSION_SID_1_A
            },
            {
                "AbstractRole": PERMISSION_ABSTRACT_ROLE_LIST_1_B,
                "Action": PERMISSION_ACTION_LIST_1_B,
                "ResourceSuffix": PERMISSION_RESOURCE_SUFFIX_LIST_1_B,
                "LogicalResourceId": PERMISSION_SID_1_B
            }
        ],
        PERMISSION_RESOURCE_ARN_2: [
            {
                "AbstractRole": PERMISSION_ABSTRACT_ROLE_LIST_2_A,
                "Action": PERMISSION_ACTION_LIST_2_A,
                "ResourceSuffix": PERMISSION_RESOURCE_SUFFIX_LIST_2_A,
                "LogicalResourceId": PERMISSION_SID_2_A
            },
            {
                "AbstractRole": PERMISSION_ABSTRACT_ROLE_LIST_2_B,
                "Action": PERMISSION_ACTION_LIST_2_B,
                "ResourceSuffix": PERMISSION_RESOURCE_SUFFIX_LIST_2_B,
                "LogicalResourceId": PERMISSION_SID_2_B
            }
        ]
    }

    ROLE_MAPPING_EFFECT_1 = 'role-mapping-effect-1'
    ROLE_MAPPING_EFFECT_2 = 'role-mapping-effect-2'
    ROLE_MAPPING_ABSTRACT_ROLE_LIST_1 = 'role-mapping-abstract-role-list-1'
    ROLE_MAPPING_ABSTRACT_ROLE_LIST_2 = 'role-mapping-abstract-role-list-2'

    ROLE_MAPPING_LIST = [
        {
            "Effect": ROLE_MAPPING_EFFECT_1,
            "AbstractRole": ROLE_MAPPING_ABSTRACT_ROLE_LIST_1
        },
        {
            "Effect": ROLE_MAPPING_EFFECT_2,
            "AbstractRole": ROLE_MAPPING_ABSTRACT_ROLE_LIST_2
        }
    ]

    @mock.patch('resource_types.Custom_AccessControl._any_abstract_roles_match')
    def test_with_no_matches(self, mock_any_abstract_roles_match):

        mock_any_abstract_roles_match.return_value = False

        expected_policy = {
            'Version': '2012-10-17',
            'Statement': []
        }

        actual_policy = Custom_AccessControl._create_role_policy(self.PERMISSIONS, self.ROLE_MAPPING_LIST)

        self.assertEquals(actual_policy, expected_policy)

        mock_any_abstract_roles_match.assert_has_calls([
            mock.call(self.PERMISSION_ABSTRACT_ROLE_LIST_1_A, self.ROLE_MAPPING_ABSTRACT_ROLE_LIST_1),
            mock.call(self.PERMISSION_ABSTRACT_ROLE_LIST_1_A, self.ROLE_MAPPING_ABSTRACT_ROLE_LIST_2),
            mock.call(self.PERMISSION_ABSTRACT_ROLE_LIST_1_B, self.ROLE_MAPPING_ABSTRACT_ROLE_LIST_1),
            mock.call(self.PERMISSION_ABSTRACT_ROLE_LIST_1_B, self.ROLE_MAPPING_ABSTRACT_ROLE_LIST_2),
            mock.call(self.PERMISSION_ABSTRACT_ROLE_LIST_2_A, self.ROLE_MAPPING_ABSTRACT_ROLE_LIST_1),
            mock.call(self.PERMISSION_ABSTRACT_ROLE_LIST_2_A, self.ROLE_MAPPING_ABSTRACT_ROLE_LIST_2),
            mock.call(self.PERMISSION_ABSTRACT_ROLE_LIST_2_B, self.ROLE_MAPPING_ABSTRACT_ROLE_LIST_1),
            mock.call(self.PERMISSION_ABSTRACT_ROLE_LIST_2_B, self.ROLE_MAPPING_ABSTRACT_ROLE_LIST_2)],
            any_order = True)


    @mock.patch('resource_types.Custom_AccessControl._any_abstract_roles_match')
    def test_with_matches(self, mock_any_abstract_roles_match):

        mock_any_abstract_roles_match.return_value = True

        expected_policy = {
            'Version': '2012-10-17',
            'Statement': [
                {
                    'Sid': self.PERMISSION_SID_1_A + '1',
                    'Effect': self.ROLE_MAPPING_EFFECT_1,
                    'Action': self.PERMISSION_ACTION_LIST_1_A,
                    'Resource': [ self.PERMISSION_RESOURCE_ARN_1 + self.PERMISSION_RESOURCE_SUFFIX_1_A_P, self.PERMISSION_RESOURCE_ARN_1 + self.PERMISSION_RESOURCE_SUFFIX_1_A_Q ]
                },
                {
                    'Sid': self.PERMISSION_SID_1_A + '2',
                    'Effect': self.ROLE_MAPPING_EFFECT_2,
                    'Action': self.PERMISSION_ACTION_LIST_1_A,
                    'Resource': [ self.PERMISSION_RESOURCE_ARN_1 + self.PERMISSION_RESOURCE_SUFFIX_1_A_P, self.PERMISSION_RESOURCE_ARN_1 + self.PERMISSION_RESOURCE_SUFFIX_1_A_Q ]
                },
                {
                    'Sid': self.PERMISSION_SID_1_B + '1',
                    'Effect': self.ROLE_MAPPING_EFFECT_1,
                    'Action': self.PERMISSION_ACTION_LIST_1_B,
                    'Resource': [ self.PERMISSION_RESOURCE_ARN_1 + self.PERMISSION_RESOURCE_SUFFIX_1_B_P, self.PERMISSION_RESOURCE_ARN_1 + self.PERMISSION_RESOURCE_SUFFIX_1_B_Q ]
                },
                {
                    'Sid': self.PERMISSION_SID_1_B + '2',
                    'Effect': self.ROLE_MAPPING_EFFECT_2,
                    'Action': self.PERMISSION_ACTION_LIST_1_B,
                    'Resource': [ self.PERMISSION_RESOURCE_ARN_1 + self.PERMISSION_RESOURCE_SUFFIX_1_B_P, self.PERMISSION_RESOURCE_ARN_1 + self.PERMISSION_RESOURCE_SUFFIX_1_B_Q ]
                },
                {
                    'Sid': self.PERMISSION_SID_2_A + '1',
                    'Effect': self.ROLE_MAPPING_EFFECT_1,
                    'Action': self.PERMISSION_ACTION_LIST_2_A,
                    'Resource': [ self.PERMISSION_RESOURCE_ARN_2 + self.PERMISSION_RESOURCE_SUFFIX_2_A_P, self.PERMISSION_RESOURCE_ARN_2 + self.PERMISSION_RESOURCE_SUFFIX_2_A_Q ]
                },
                {
                    'Sid': self.PERMISSION_SID_2_A + '2',
                    'Effect': self.ROLE_MAPPING_EFFECT_2,
                    'Action': self.PERMISSION_ACTION_LIST_2_A,
                    'Resource': [ self.PERMISSION_RESOURCE_ARN_2 + self.PERMISSION_RESOURCE_SUFFIX_2_A_P, self.PERMISSION_RESOURCE_ARN_2 + self.PERMISSION_RESOURCE_SUFFIX_2_A_Q ]
                },
                {
                    'Sid': self.PERMISSION_SID_2_B + '1',
                    'Effect': self.ROLE_MAPPING_EFFECT_1,
                    'Action': self.PERMISSION_ACTION_LIST_2_B,
                    'Resource': [ self.PERMISSION_RESOURCE_ARN_2 + self.PERMISSION_RESOURCE_SUFFIX_2_B_P, self.PERMISSION_RESOURCE_ARN_2 + self.PERMISSION_RESOURCE_SUFFIX_2_B_Q ]
                },
                {
                    'Sid': self.PERMISSION_SID_2_B + '2',
                    'Effect': self.ROLE_MAPPING_EFFECT_2,
                    'Action': self.PERMISSION_ACTION_LIST_2_B,
                    'Resource': [ self.PERMISSION_RESOURCE_ARN_2 + self.PERMISSION_RESOURCE_SUFFIX_2_B_P, self.PERMISSION_RESOURCE_ARN_2 + self.PERMISSION_RESOURCE_SUFFIX_2_B_Q ]
                },
            ]
        }

        actual_policy = Custom_AccessControl._create_role_policy(self.PERMISSIONS, self.ROLE_MAPPING_LIST)

        self.assertItemsEqual(actual_policy['Statement'], expected_policy['Statement'])

        mock_any_abstract_roles_match.assert_has_calls([
            mock.call(self.PERMISSION_ABSTRACT_ROLE_LIST_1_A, self.ROLE_MAPPING_ABSTRACT_ROLE_LIST_1),
            mock.call(self.PERMISSION_ABSTRACT_ROLE_LIST_1_A, self.ROLE_MAPPING_ABSTRACT_ROLE_LIST_2),
            mock.call(self.PERMISSION_ABSTRACT_ROLE_LIST_1_B, self.ROLE_MAPPING_ABSTRACT_ROLE_LIST_1),
            mock.call(self.PERMISSION_ABSTRACT_ROLE_LIST_1_B, self.ROLE_MAPPING_ABSTRACT_ROLE_LIST_2),
            mock.call(self.PERMISSION_ABSTRACT_ROLE_LIST_2_A, self.ROLE_MAPPING_ABSTRACT_ROLE_LIST_1),
            mock.call(self.PERMISSION_ABSTRACT_ROLE_LIST_2_A, self.ROLE_MAPPING_ABSTRACT_ROLE_LIST_2),
            mock.call(self.PERMISSION_ABSTRACT_ROLE_LIST_2_B, self.ROLE_MAPPING_ABSTRACT_ROLE_LIST_1),
            mock.call(self.PERMISSION_ABSTRACT_ROLE_LIST_2_B, self.ROLE_MAPPING_ABSTRACT_ROLE_LIST_2)],
            any_order = True)


class UnitTest_CloudGemFramework_ProjectResourceHandler_Custom_AccessControl_any_abstract_roles_match(Custom_AccessControlTestCase):

    def test_with_no_match(self):

        permission_abstract_role_list = [ 
            [ 'resource-group-1', u'abstract-role-a' ], 
            [ 'resource-group-1', u'abstract-role-b' ], 
            [ 'resource-group-2', u'abstract-role-a' ], 
            [ 'resource-group-2', u'abstract-role-b' ] ]
        
        mapping_abstract_role_list = [ 
            [ 'resource-group-1', 'abstract-role-x' ],
            [ 'resource-group-x', 'abstract-role-a' ] ]

        actual = Custom_AccessControl._any_abstract_roles_match(permission_abstract_role_list, mapping_abstract_role_list)

        self.assertFalse(actual)


    def test_with_exact_match(self):

        permission_abstract_role_list = [ 
            [ 'resource-group-1', u'abstract-role-a' ], 
            [ 'resource-group-1', u'abstract-role-b' ], 
            [ 'resource-group-2', u'abstract-role-a' ], 
            [ 'resource-group-2', u'abstract-role-b' ] ]
        
        mapping_abstract_role_list = [ [ 'resource-group-2', 'abstract-role-a' ] ]

        actual = Custom_AccessControl._any_abstract_roles_match(permission_abstract_role_list, mapping_abstract_role_list)

        self.assertTrue(actual)


    def test_with_wildcard_match(self):

        permission_abstract_role_list = [ 
            [ 'resource-group-1', u'abstract-role-a' ], 
            [ 'resource-group-1', u'abstract-role-b' ], 
            [ 'resource-group-2', u'abstract-role-a' ], 
            [ 'resource-group-2', u'abstract-role-b' ] ]
        
        mapping_abstract_role_list = [ [ '*', 'abstract-role-a' ] ]

        actual = Custom_AccessControl._any_abstract_roles_match(permission_abstract_role_list, mapping_abstract_role_list)

        self.assertTrue(actual)


if __name__ == '__main__':
    unittest.main()
