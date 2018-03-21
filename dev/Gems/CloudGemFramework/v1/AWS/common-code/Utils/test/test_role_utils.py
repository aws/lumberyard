import unittest
import mock
from cgf_utils.test import mock_aws
from cgf_utils import role_utils
import time
from resource_manager_common import stack_info
from resource_manager_common.stack_info import StackInfo

from botocore.exceptions import ClientError

_STACK_INFO_MANAGER = stack_info.StackInfoManager()


class JSONStringEqualsDict(object):
    def __init__(self, dict):
        self.__dict = dict

    def __eq__(self, json_string):
        import json
        other_dict = json.loads(json_string)
        return self.__dict == other_dict


class UnitTest_CloudGemFramework_ProjectResourceHandler_role_utils_get_role_name(unittest.TestCase):
    def test_with_short_name(self):
        stack_arn = 'arn:aws:cloudformation:TestRegion:TestAccount:stack/ShortName-RandomPart/TestUUID'
        logical_role_name = 'RoleName'
        actual_role_name = role_utils.get_access_control_role_name(stack_arn, logical_role_name)
        self.assertEqual(actual_role_name, 'ShortName-RandomPart-RoleName')

    def test_with_too_long_name(self):
        stack_arn = 'arn:aws:cloudformation:TestRegion:TestAccount:stack/LongName9A123456789B123456789C123456789D123456789E123456789F123456789G-RandomPart/TestUUID'
        logical_role_name = 'RoleName9A123456789B123456789C123456789D123456789E123456789F123456789G'
        actual_role_name = role_utils.get_access_control_role_name(stack_arn, logical_role_name)
        self.assertEqual(actual_role_name, role_utils.sanitize_role_name('LongName9A123456789B123456789C123456789D123456789E123456789F123456789G-RandomPart-RoleName9A123456789B123456789C123456789D123456789E123456789F123456789G'))


class UnitTest_CloudGemFramework_ProjectResourceHandler_role_utils_create_access_control_role(unittest.TestCase):
    project_name = 'test-project-name'
    deployment_name = 'test-deployment-name'
    resource_group_name = 'test-resource-group-name'

    stack_arn = 'arn:aws:cloudformation:TestRegion:TestAccount:stack/ShortName-RandomPart/TestUUID'
    logical_role_name = 'test-logical-role-name'

    assume_role_service = 'test-assume-role-service'

    role_name = 'test-role-name'

    path = '/{project_name}/{deployment_name}/{resource_group_name}/{logical_role_name}/AccessControl/'.format(
        project_name=project_name,
        deployment_name=deployment_name,
        resource_group_name=resource_group_name,
        logical_role_name=logical_role_name)

    mock_stack_info = mock.MagicMock()
    mock_stack_info.deployment.project.project_name = project_name
    mock_stack_info.deployment.deployment_name = deployment_name
    mock_stack_info.resource_group_name = resource_group_name
    mock_stack_info.parameters = {'CloudCanvasStack': StackInfo.STACK_TYPE_RESOURCE_GROUP}
    mock_stack_info.stack_name = project_name
    mock_stack_info.stack_type = StackInfo.STACK_TYPE_RESOURCE_GROUP

    role_arn = 'test-role-arn'

    assume_role_policy_document = {
        "Version": "2012-10-17",
        "Statement": [
            {
                "Effect": "Allow",
                "Action": "sts:AssumeRole",
                "Principal": {
                    "Service": assume_role_service
                }
            }
        ]
    }

    assume_role_policy_document_matcher = JSONStringEqualsDict(assume_role_policy_document)

    create_role_response = {
        'Role': {
            'Arn': role_arn
        }
    }

    default_policy = 'test-policy'

    @mock_aws.patch_client('iam', 'create_role', return_value=create_role_response, reload=role_utils)
    @mock.patch('cgf_utils.role_utils.get_access_control_role_name', return_value=role_name)
    @mock.patch('resource_manager_common.stack_info.StackInfoManager.get_stack_info', return_value=mock_stack_info)
    def test_default(self, mock_get_stack_info, mock_get_access_control_role_name, mock_create_role):
        id_data = {}
        role_utils.PROPAGATION_DELAY_SECONDS = 2
        start_time = time.clock()
        created_role_arn = role_utils.create_access_control_role(
            _STACK_INFO_MANAGER,
            id_data,
            self.stack_arn,
            self.logical_role_name,
            self.assume_role_service)
        stop_time = time.clock()
        delay_time = stop_time - start_time
        self.assertAlmostEqual(delay_time, role_utils.PROPAGATION_DELAY_SECONDS, delta=0.1)
        self.assertEqual(created_role_arn, self.role_arn)
        self.assertEquals(id_data, {'AbstractRoleMappings': {self.logical_role_name: self.role_name}})
        mock_get_access_control_role_name.assert_called_once_with(self.stack_arn, self.logical_role_name)
        mock_create_role.assert_called_once_with(RoleName=self.role_name,
                                                 AssumeRolePolicyDocument=self.assume_role_policy_document_matcher,
                                                 Path=self.path)
        mock_get_stack_info.assert_called_once_with(self.stack_arn)

    @mock_aws.patch_client('iam', 'create_role', return_value=create_role_response, reload=role_utils)
    @mock.patch('cgf_utils.role_utils.get_access_control_role_name', return_value=role_name)
    @mock.patch('resource_manager_common.stack_info.StackInfoManager.get_stack_info', return_value=mock_stack_info)
    def test_with_delay_for_propagation_false(self, mock_get_stack_info, mock_get_access_control_role_name,
                                              mock_create_role):
        id_data = {}
        role_utils.PROPAGATION_DELAY_SECONDS = 2
        start_time = time.clock()
        created_role_arn = role_utils.create_access_control_role(
            _STACK_INFO_MANAGER,
            id_data,
            self.stack_arn,
            self.logical_role_name,
            self.assume_role_service,
            delay_for_propagation=False)
        stop_time = time.clock()
        delay_time = stop_time - start_time
        self.assertAlmostEqual(delay_time, 0, delta=0.1)
        self.assertEqual(created_role_arn, self.role_arn)
        self.assertEquals(id_data, {'AbstractRoleMappings': {self.logical_role_name: self.role_name}})
        mock_get_access_control_role_name.assert_called_once_with(self.stack_arn, self.logical_role_name)
        mock_create_role.assert_called_once_with(RoleName=self.role_name,
                                                 AssumeRolePolicyDocument=self.assume_role_policy_document_matcher,
                                                 Path=self.path)
        mock_get_stack_info.assert_called_once_with(self.stack_arn)

    @mock_aws.patch_client('iam', 'create_role', return_value=create_role_response, reload=role_utils)
    @mock_aws.patch_client('iam', 'put_role_policy', return_value={})
    @mock.patch('cgf_utils.role_utils.get_access_control_role_name', return_value=role_name)
    @mock.patch('resource_manager_common.stack_info.StackInfoManager.get_stack_info', return_value=mock_stack_info)
    def test_with_default_policy(self, mock_get_stack_info, mock_get_access_control_role_name, mock_put_role_policy,
                                 mock_create_role):
        id_data = {}
        created_role_arn = role_utils.create_access_control_role(
            _STACK_INFO_MANAGER,
            id_data,
            self.stack_arn,
            self.logical_role_name,
            self.assume_role_service,
            delay_for_propagation=False,
            default_policy=self.default_policy)
        self.assertEqual(created_role_arn, self.role_arn)
        self.assertEquals(id_data, {'AbstractRoleMappings': {self.logical_role_name: self.role_name}})
        mock_get_access_control_role_name.assert_called_once_with(self.stack_arn, self.logical_role_name)
        mock_create_role.assert_called_once_with(RoleName=self.role_name,
                                                 AssumeRolePolicyDocument=self.assume_role_policy_document_matcher,
                                                 Path=self.path)
        mock_put_role_policy.assert_called_once_with(RoleName=self.role_name, PolicyName='Default',
                                                     PolicyDocument=self.default_policy)
        mock_get_stack_info.assert_called_once_with(self.stack_arn)


class UnitTest_CloudGemFramework_ProjectResourceHandler_role_utils_delete_access_control_role(unittest.TestCase):
    policy_name_1 = 'policy_name_1'
    policy_name_2 = 'policy_name_2'

    list_role_policies_response = {
        'PolicyNames': [policy_name_1, policy_name_2]
    }

    delete_role_policy_response = {
    }

    delete_role_response = {
    }

    no_such_entity_response = {
        'Error': {
            'Code': 'NoSuchEntity'
        }
    }

    access_denied_response = {
        'Error': {
            'Code': 'AccessDenied'
        }
    }

    unknown_error_code = 'test-error-code'

    other_error_response = {
        'Error': {
            'Code': unknown_error_code
        }
    }

    resource_group_stack_arn = 'test-resource-group-stack-arn'
    logical_role_name = 'test-logical-role-name'

    role_name = 'test-role-name'

    @mock_aws.patch_client('iam', 'list_role_policies', return_value=list_role_policies_response, reload=role_utils)
    @mock_aws.patch_client('iam', 'delete_role_policy', return_value=delete_role_policy_response)
    @mock_aws.patch_client('iam', 'delete_role', return_value=delete_role_response)
    def test_with_no_errors(self, mock_delete_role, mock_delete_role_policy, mock_list_role_policies):
        id_data = {'AbstractRoleMappings': {self.logical_role_name: self.role_name}}
        role_utils.delete_access_control_role(id_data, self.logical_role_name)
        self.assertEquals(id_data, {'AbstractRoleMappings': {}})
        mock_list_role_policies.assert_called_once_with(RoleName=self.role_name)
        mock_delete_role_policy.assert_any_call(RoleName=self.role_name, PolicyName=self.policy_name_1)
        mock_delete_role_policy.assert_any_call(RoleName=self.role_name, PolicyName=self.policy_name_2)
        mock_delete_role.assert_called_once_with(RoleName=self.role_name)

    @mock_aws.patch_client('iam', 'list_role_policies',
                           side_effect=ClientError(no_such_entity_response, 'list_role_policies'), reload=role_utils)
    @mock_aws.patch_client('iam', 'delete_role_policy', return_value=delete_role_policy_response)
    @mock_aws.patch_client('iam', 'delete_role', return_value=delete_role_response)
    def test_with_list_role_policies_no_such_entity_error(self, mock_delete_role, mock_delete_role_policy,
                                                          mock_list_role_policies):
        id_data = {'AbstractRoleMappings': {self.logical_role_name: self.role_name}}
        role_utils.delete_access_control_role(id_data, self.logical_role_name)
        self.assertEquals(id_data, {'AbstractRoleMappings': {}})
        mock_list_role_policies.assert_called_once_with(RoleName=self.role_name)
        mock_delete_role.assert_called_once_with(RoleName=self.role_name)

    @mock_aws.patch_client('iam', 'list_role_policies',
                           side_effect=ClientError(access_denied_response, 'list_role_policies'), reload=role_utils)
    @mock_aws.patch_client('iam', 'delete_role_policy', return_value=delete_role_policy_response)
    @mock_aws.patch_client('iam', 'delete_role', return_value=delete_role_response)
    def test_with_list_role_policies_access_denied_error(self, mock_delete_role, mock_delete_role_policy,
                                                         mock_list_role_policies):
        id_data = {'AbstractRoleMappings': {self.logical_role_name: self.role_name}}
        role_utils.delete_access_control_role(id_data, self.logical_role_name)
        self.assertEquals(id_data, {'AbstractRoleMappings': {}})
        mock_list_role_policies.assert_called_once_with(RoleName=self.role_name)
        mock_delete_role.assert_called_once_with(RoleName=self.role_name)

    @mock_aws.patch_client('iam', 'list_role_policies',
                           side_effect=ClientError(other_error_response, 'list_role_policies'), reload=role_utils)
    @mock_aws.patch_client('iam', 'delete_role_policy', return_value=delete_role_policy_response)
    @mock_aws.patch_client('iam', 'delete_role', return_value=delete_role_response)
    def test_with_list_role_policies_other_error(self, mock_list_role_policies, mock_delete_role,
                                                 mock_delete_role_policy):
        id_data = {'AbstractRoleMappings': {self.logical_role_name: self.role_name}}
        with self.assertRaisesRegexp(ClientError, self.unknown_error_code):
            role_utils.delete_access_control_role(id_data, self.logical_role_name)
        self.assertEquals(id_data, {'AbstractRoleMappings': {self.logical_role_name: self.role_name}})
        mock_list_role_policies.assert_called_once_with(RoleName=self.role_name)

    @mock_aws.patch_client('iam', 'list_role_policies', return_value=list_role_policies_response, reload=role_utils)
    @mock_aws.patch_client('iam', 'delete_role_policy',
                           side_effect=ClientError(no_such_entity_response, 'delete_role_policy'))
    @mock_aws.patch_client('iam', 'delete_role', return_value=delete_role_response)
    def test_with_delete_role_policy_no_such_entity_error(self, mock_delete_role, mock_delete_role_policy,
                                                          mock_list_role_policies):
        id_data = {'AbstractRoleMappings': {self.logical_role_name: self.role_name}}
        role_utils.delete_access_control_role(id_data, self.logical_role_name)
        self.assertEquals(id_data, {'AbstractRoleMappings': {}})
        mock_list_role_policies.assert_called_once_with(RoleName=self.role_name)
        mock_delete_role_policy.assert_any_call(RoleName=self.role_name, PolicyName=self.policy_name_1)
        mock_delete_role_policy.assert_any_call(RoleName=self.role_name, PolicyName=self.policy_name_2)
        mock_delete_role.assert_called_once_with(RoleName=self.role_name)

    @mock_aws.patch_client('iam', 'list_role_policies', return_value=list_role_policies_response, reload=role_utils)
    @mock_aws.patch_client('iam', 'delete_role_policy',
                           side_effect=ClientError(access_denied_response, 'delete_role_policy'))
    @mock_aws.patch_client('iam', 'delete_role', return_value=delete_role_response)
    def test_with_delete_role_policy_access_denied_error(self, mock_delete_role, mock_delete_role_policy,
                                                         mock_list_role_policies):
        id_data = {'AbstractRoleMappings': {self.logical_role_name: self.role_name}}
        role_utils.delete_access_control_role(id_data, self.logical_role_name)
        self.assertEquals(id_data, {'AbstractRoleMappings': {}})
        mock_list_role_policies.assert_called_once_with(RoleName=self.role_name)
        mock_delete_role_policy.assert_any_call(RoleName=self.role_name, PolicyName=self.policy_name_1)
        mock_delete_role_policy.assert_any_call(RoleName=self.role_name, PolicyName=self.policy_name_2)
        mock_delete_role.assert_called_once_with(RoleName=self.role_name)

    @mock_aws.patch_client('iam', 'list_role_policies', return_value=list_role_policies_response, reload=role_utils)
    @mock_aws.patch_client('iam', 'delete_role_policy',
                           side_effect=ClientError(other_error_response, 'delete_role_policy'))
    @mock_aws.patch_client('iam', 'delete_role', return_value=delete_role_response)
    def test_with_delete_role_policy_other_error(self, mock_list_role_policies, mock_delete_role_policy,
                                                 mock_delete_role):
        print '===', mock_list_role_policies, mock_delete_role, mock_delete_role_policy
        id_data = {'AbstractRoleMappings': {self.logical_role_name: self.role_name}}
        with self.assertRaisesRegexp(ClientError, self.unknown_error_code):
            role_utils.delete_access_control_role(id_data, self.logical_role_name)
        self.assertEquals(id_data, {'AbstractRoleMappings': {self.logical_role_name: self.role_name}})
        mock_delete_role_policy.assert_any_call(RoleName=self.role_name, PolicyName=self.policy_name_1)
        mock_list_role_policies.assert_called_once_with(RoleName=self.role_name)

    @mock_aws.patch_client('iam', 'list_role_policies', return_value=list_role_policies_response, reload=role_utils)
    @mock_aws.patch_client('iam', 'delete_role_policy', return_value=delete_role_policy_response)
    @mock_aws.patch_client('iam', 'delete_role', side_effect=ClientError(no_such_entity_response, 'delete_role'))
    def test_with_delete_role_no_such_entity_error(self, mock_delete_role, mock_delete_role_policy,
                                                   mock_list_role_policies):
        id_data = {'AbstractRoleMappings': {self.logical_role_name: self.role_name}}
        role_utils.delete_access_control_role(id_data, self.logical_role_name)
        self.assertEquals(id_data, {'AbstractRoleMappings': {}})
        mock_list_role_policies.assert_called_once_with(RoleName=self.role_name)
        mock_delete_role_policy.assert_any_call(RoleName=self.role_name, PolicyName=self.policy_name_1)
        mock_delete_role_policy.assert_any_call(RoleName=self.role_name, PolicyName=self.policy_name_2)
        mock_delete_role.assert_called_once_with(RoleName=self.role_name)

    @mock_aws.patch_client('iam', 'list_role_policies', return_value=list_role_policies_response, reload=role_utils)
    @mock_aws.patch_client('iam', 'delete_role_policy', return_value=delete_role_policy_response)
    @mock_aws.patch_client('iam', 'delete_role', side_effect=ClientError(access_denied_response, 'delete_role'))
    def test_with_delete_role_access_denied_error(self, mock_delete_role, mock_delete_role_policy,
                                                  mock_list_role_policies):
        id_data = {'AbstractRoleMappings': {self.logical_role_name: self.role_name}}
        role_utils.delete_access_control_role(id_data, self.logical_role_name)
        self.assertEquals(id_data, {'AbstractRoleMappings': {}})
        mock_list_role_policies.assert_called_once_with(RoleName=self.role_name)
        mock_delete_role_policy.assert_any_call(RoleName=self.role_name, PolicyName=self.policy_name_1)
        mock_delete_role_policy.assert_any_call(RoleName=self.role_name, PolicyName=self.policy_name_2)
        mock_delete_role.assert_called_once_with(RoleName=self.role_name)

    @mock_aws.patch_client('iam', 'list_role_policies', return_value=list_role_policies_response, reload=role_utils)
    @mock_aws.patch_client('iam', 'delete_role_policy', return_value=delete_role_policy_response)
    @mock_aws.patch_client('iam', 'delete_role', side_effect=ClientError(other_error_response, 'delete_role'))
    def test_with_delete_role_other_error(self, mock_delete_role, mock_delete_role_policy, mock_list_role_policies):
        id_data = {'AbstractRoleMappings': {self.logical_role_name: self.role_name}}
        with self.assertRaisesRegexp(ClientError, self.unknown_error_code):
            role_utils.delete_access_control_role(id_data, self.logical_role_name)
        self.assertEquals(id_data, {'AbstractRoleMappings': {self.logical_role_name: self.role_name}})
        mock_delete_role_policy.assert_any_call(RoleName=self.role_name, PolicyName=self.policy_name_1)
        mock_delete_role_policy.assert_any_call(RoleName=self.role_name, PolicyName=self.policy_name_2)
        mock_list_role_policies.assert_called_once_with(RoleName=self.role_name)


class UnitTest_CloudGemFramework_ProjectResourceHandler_role_utils_get_access_control_role_arn(unittest.TestCase):
    resource_group_stack_arn = 'test-resource-group-stack-arn'
    logical_role_name = 'test-logical-role-name'

    role_name = 'test-role-name'

    role_arn = 'test-role-arn'

    get_role_response = {
        'Role': {
            'Arn': role_arn
        }
    }

    @mock_aws.patch_client('iam', 'get_role', return_value=get_role_response, reload=role_utils)
    def test_default(self, mock_get_role):
        id_data = {'AbstractRoleMappings': {self.logical_role_name: self.role_name}}
        actual_role_arn = role_utils.get_access_control_role_arn(id_data, self.logical_role_name)
        self.assertEquals(actual_role_arn, self.role_arn)
        mock_get_role.assert_called_once_with(RoleName=self.role_name)


