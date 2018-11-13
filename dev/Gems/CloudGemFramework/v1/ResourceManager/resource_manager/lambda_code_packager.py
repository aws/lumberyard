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
# $Revision$

import boto3

import uploader
import copy
import os
import common_code
import file_util
import zipfile

from errors import HandledError


from resource_manager_common import constant

UPLOADED_FILE_FORMAT = "{}-lambda-code.zip"

class LambdaCodePackageValidator(object):
    def __init__(self, context, function_name):
        self.local_zip_path = None
        self.context = context
        self.function_name = function_name
        self.runtime = "UNKNOWN"

    def validate_package(self, additional_info):
        code_paths = additional_info["codepaths"]
        source_directory_path = self._get_single_code_path(code_paths)
        zip_path = os.path.join(
            source_directory_path, constant.LAMBDA_CODE_DIRECTORY_NAME, self.function_name + '.zip')

        if os.path.exists(zip_path):
            self.local_zip_path = zip_path

    def _get_single_code_path(self, code_paths, allow_multiple_paths=False):
        if len(code_paths) > 1 and not allow_multiple_paths:
            raise HandledError("Multiple code paths were found, this is not allowed for runtime {}".format(self.runtime))
        return code_paths[0]

    def upload(self, uploader):
        if not self.local_zip_path:
            raise HandledError("A local zip was not found")
        print "uploading {}".format(self.local_zip_path)
        return uploader.upload_file(UPLOADED_FILE_FORMAT.format(self.function_name), self.local_zip_path)["key"]

class NodeLambdaPackageValidator(LambdaCodePackageValidator):
    def __init__(self, context, function_name):
        super(NodeLambdaPackageValidator, self).__init__(context, function_name)
        self.runtime = "node.js"

class JavaCodePackageValidator(LambdaCodePackageValidator):
    def __init__(self, context, function_name):
        super(JavaCodePackageValidator, self).__init__(context, function_name)
        self.runtime = "Java"

    def validate_package(self, additional_info):
        code_paths = additional_info["codepaths"]
        source_directory_path = self._get_single_code_path(code_paths)

        # check for .jar
        jar_path = os.path.join(
            source_directory_path, constant.LAMBDA_CODE_DIRECTORY_NAME, self.function_name + '.jar')
        if os.path.exists(jar_path):
            self.local_zip_path = jar_path
            return

        # Eclipse using maven spits it out as <project>-SNAPSHOT.jar no reason not to support this
        jar_path = os.path.join(
            source_directory_path, constant.LAMBDA_CODE_DIRECTORY_NAME, self.function_name + '-SNAPSHOT.jar ')
        if os.path.exists(jar_path):
            self.local_zip_path = jar_path
            return
        
        zip_path = os.path.join(
            source_directory_path, constant.LAMBDA_CODE_DIRECTORY_NAME, self.function_name + '.zip')

        if not os.path.exists(zip_path):
            return
        self.local_zip_path = zip_path

        zf = zipfile.ZipFile(zip_path)
        file_list = zf.namelist()
        ''' 
        From https://docs.aws.amazon.com/lambda/latest/dg/create-deployment-pkg-zip-java.html
        The .zip file must have the following structure:
            - All compiled class files and resource files at the root level.
            - All required jars to run the code in the /lib directory.
        '''
        for file_name in file_list:
            if file_name.endswith(".jar"):
                if file_name.split("/")[1] != "lib":
                    raise HandledError(
                        "Java package {} does not follow the structure rule: All required jars to run the code in the /lib directory.".format(zip_path))


class DotNetCodePackageValidator(LambdaCodePackageValidator):
    def __init__(self, context, function_name):
        super(DotNetCodePackageValidator, self).__init__(context, function_name)
        self.runtime = "dotnet"

    def validate_package(self, additional_info):
        code_paths = additional_info["codepaths"]
        source_directory_path = self._get_single_code_path(code_paths)

        zip_path = os.path.join(
            source_directory_path, constant.LAMBDA_CODE_DIRECTORY_NAME, self.function_name + '.zip')

        if not os.path.exists(zip_path):
            return
        self.local_zip_path = zip_path

        zf = zipfile.ZipFile(zip_path)
        file_list = zf.namelist()
        ''' 
        Create a C# lambda using the instructions here https://aws.amazon.com/blogs/developer/creating-net-core-aws-lambda-projects-without-visual-studio/
        Make the zip using `dotnet lambda package`
        '''
        has_deps_file = False
        expected_files = ['Amazon.Lambda.Core.dll']
        for file_name in file_list:
            if file_name in expected_files:
                expected_files.remove(file_name)
            if file_name.endswith(".deps.json"):
                has_deps_file = True

        if expected_files:
            raise HandledError("Missing expected files {}".format(expected_files))
        if not has_deps_file:
            raise HandledError("Missing deps.json file")

class GolangCodePackageValidator(LambdaCodePackageValidator):
    def __init__(self, context, function_name):
        super(GolangCodePackageValidator, self).__init__(context, function_name)
        self.runtime = "Go"


