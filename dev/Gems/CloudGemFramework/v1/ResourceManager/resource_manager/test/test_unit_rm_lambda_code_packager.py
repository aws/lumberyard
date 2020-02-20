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
# $Revision: #1 $

import mock
import unittest

from resource_manager import errors
from resource_manager import lambda_code_packager


def make_os_path_patcher(side_effect):
    """Patch up os.path.exists with the side_effects required"""
    patcher = mock.patch('os.path.exists')
    mock_os_exists = patcher.start()
    mock_os_exists.side_effect = side_effect
    return mock_os_exists


class UnitTest_BaseLambdaCodePackager(unittest.TestCase):
    TEST_DIRECTORY_PATH = 'TestDirectory'
    TEST_FUNCTION_NAME = "TestFunctionName"

    def __init__(self, methodName='runTest'):
        super(UnitTest_BaseLambdaCodePackager, self).__init__(methodName)
        self.uploader = mock.MagicMock()
        self.context = mock.MagicMock()
        self.additional_info = {'codepaths': [self.TEST_DIRECTORY_PATH]}


class UnitTest_CloudGemFramework_ResourceManager_GoLambdaCodePackager(UnitTest_BaseLambdaCodePackager):

    def test_given_go_lang_packager_with_valid_code_path_when_validate_then_pass(self):
        make_os_path_patcher([True])

        packager = lambda_code_packager.GolangCodePackageValidator(self.context, self.TEST_FUNCTION_NAME)
        self.assertTrue(packager.validate_package(additional_info=self.additional_info))

    def test_given_go_lang_packager_with_invalid_code_path_when_validate_then_pass(self):
        make_os_path_patcher([False])

        packager = lambda_code_packager.GolangCodePackageValidator(self.context, self.TEST_FUNCTION_NAME)
        self.assertFalse(packager.validate_package(additional_info=self.additional_info))

    def test_given_go_lang_packager_with_bad_zippath_when_upload_then_exception(self):
        packager = lambda_code_packager.GolangCodePackageValidator(self.context, self.TEST_FUNCTION_NAME)
        self.assertRaises(errors.HandledError, packager.upload, uploader=self.uploader)


class UnitTest_CloudGemFramework_ResourceManager_NodeLambdaPackageValidator(UnitTest_BaseLambdaCodePackager):

    def test_given_node_lang_packager_with_valid_code_path_when_validate_then_pass(self):
        make_os_path_patcher([True])

        packager = lambda_code_packager.NodeLambdaPackageValidator(self.context, self.TEST_FUNCTION_NAME)
        self.assertTrue(packager.validate_package(additional_info=self.additional_info))

    def test_given_node_lang_packager_with_invalid_code_path_when_validate_then_pass(self):
        make_os_path_patcher([False])

        packager = lambda_code_packager.NodeLambdaPackageValidator(self.context, self.TEST_FUNCTION_NAME)
        self.assertFalse(packager.validate_package(additional_info=self.additional_info))

    def test_given_node_lang_packager_with_bad_zippath_when_upload_then_exception(self):
        packager = lambda_code_packager.NodeLambdaPackageValidator(self.context, self.TEST_FUNCTION_NAME)
        self.assertRaises(errors.HandledError, packager.upload, uploader=self.uploader)


class UnitTest_CloudGemFramework_ResourceManager_DotNetLambdaCodePackager(UnitTest_BaseLambdaCodePackager):

    def test_given_dot_net_packager_wth_valid_zip_when_validate_then_pass(self):
        make_os_path_patcher([True])

        packager = lambda_code_packager.DotNetCodePackageValidator(self.context, self.TEST_FUNCTION_NAME)
        # Create mock zipfile and override the is_zipfile function
        with mock.patch('resource_manager.lambda_code_packager.zipfile') as mock_zipfile:
            mock_zipfile.is_zipfile.return_value = True
            mock_zipfile.namelist.return_value = ['Amazon.Lambda.Core.dll', 'TestLambda.deps.json']

            # Since a ZipFile is a separate object, which returns a zipfile (note
            # that that's lowercase), we need to mock the ZipFile and have it return
            # the zipfile mock previously created.
            with mock.patch('resource_manager.lambda_code_packager.zipfile.ZipFile') as mock_ZipFile:
                mock_ZipFile.return_value = mock_zipfile

                self.assertTrue(packager.validate_package(additional_info=self.additional_info))

    def test_given_dot_net_packager_with_zip_without_required_dll_when_validate_then_exception(self):
        make_os_path_patcher([True])

        packager = lambda_code_packager.DotNetCodePackageValidator(self.context, self.TEST_FUNCTION_NAME)
        # Create mock zipfile and override the is_zipfile function
        with mock.patch('resource_manager.lambda_code_packager.zipfile') as mock_zipfile:
            mock_zipfile.is_zipfile.return_value = True
            mock_zipfile.namelist.return_value = ['TestLambda.deps.json']

            # Since a ZipFile is a separate object, which returns a zipfile (note
            # that that's lowercase), we need to mock the ZipFile and have it return
            # the zipfile mock previously created.
            with mock.patch('resource_manager.lambda_code_packager.zipfile.ZipFile') as mock_ZipFile:
                mock_ZipFile.return_value = mock_zipfile

                self.assertRaises(errors.HandledError, packager.validate_package, additional_info=self.additional_info)

    def test_given_dot_net_packager_with_zip_without_required_deps_file_when_validate_then_exception(self):
        make_os_path_patcher([True])

        packager = lambda_code_packager.DotNetCodePackageValidator(self.context, self.TEST_FUNCTION_NAME)
        # Create mock zipfile and override the is_zipfile function
        with mock.patch('resource_manager.lambda_code_packager.zipfile') as mock_zipfile:
            mock_zipfile.is_zipfile.return_value = True
            mock_zipfile.namelist.return_value = ['Amazon.Lambda.Core.dll']

            # Since a ZipFile is a separate object, which returns a zipfile (note
            # that that's lowercase), we need to mock the ZipFile and have it return
            # the zipfile mock previously created.
            with mock.patch('resource_manager.lambda_code_packager.zipfile.ZipFile') as mock_ZipFile:
                mock_ZipFile.return_value = mock_zipfile

                self.assertRaises(errors.HandledError, packager.validate_package, additional_info=self.additional_info)

    def test_given_dot_net_lang_packager_with_invalid_code_path_when_validate_then_fail(self):
        make_os_path_patcher([False])

        packager = lambda_code_packager.DotNetCodePackageValidator(self.context, self.TEST_FUNCTION_NAME)
        self.assertFalse(packager.validate_package(additional_info=self.additional_info))

    def test_given_dot_net_packager_with_bad_zippath_when_upload_then_exception(self):
        packager = lambda_code_packager.DotNetCodePackageValidator(self.context, self.TEST_FUNCTION_NAME)
        self.assertRaises(errors.HandledError, packager.upload, uploader=self.uploader)


class UnitTest_CloudGemFramework_ResourceManager_JavaLambdaCodePackager(UnitTest_BaseLambdaCodePackager):

    def test_given_java_packager_wth_valid_base_jar_path_when_validate_then_pass(self):
        make_os_path_patcher([True])  # has base jar

        packager = lambda_code_packager.JavaCodePackageValidator(self.context, self.TEST_FUNCTION_NAME)
        self.assertTrue(packager.validate_package(additional_info=self.additional_info))

    def test_given_java_packager_wth_valid_snapshot_jar_path_when_validate_then_pass(self):
        make_os_path_patcher([False, True])  # No base jar, but snapshot jar

        packager = lambda_code_packager.JavaCodePackageValidator(self.context, self.TEST_FUNCTION_NAME)
        self.assertTrue(packager.validate_package(additional_info=self.additional_info))

    def test_given_java_packager_with_zip_without_bad_structure_file_when_validate_then_exception(self):
        make_os_path_patcher([False, False, True])  # No base jar, snapshot jar but zip

        packager = lambda_code_packager.JavaCodePackageValidator(self.context, self.TEST_FUNCTION_NAME)
        # Create mock zipfile and override the is_zipfile function
        with mock.patch('resource_manager.lambda_code_packager.zipfile') as mock_zipfile:
            mock_zipfile.is_zipfile.return_value = True
            mock_zipfile.namelist.return_value = ['some_path/fail.jar']  # All jars need to be in the lib/ folder

            # Since a ZipFile is a separate object, which returns a zipfile (note
            # that that's lowercase), we need to mock the ZipFile and have it return
            # the zipfile mock previously created.
            with mock.patch('resource_manager.lambda_code_packager.zipfile.ZipFile') as mock_ZipFile:
                mock_ZipFile.return_value = mock_zipfile

                self.assertRaises(errors.HandledError, packager.validate_package, additional_info=self.additional_info)

    def test_given_java_lang_packager_with_invalid_code_path_when_validate_then_fail(self):
        make_os_path_patcher([False, False, False])  # No base jar, snapshot jar or zip

        packager = lambda_code_packager.JavaCodePackageValidator(self.context, self.TEST_FUNCTION_NAME)
        self.assertFalse(packager.validate_package(additional_info=self.additional_info))

    def test_given_java_packager_with_bad_zippath_when_upload_then_exception(self):
        packager = lambda_code_packager.JavaCodePackageValidator(self.context, self.TEST_FUNCTION_NAME)
        self.assertRaises(errors.HandledError, packager.upload, uploader=self.uploader)
