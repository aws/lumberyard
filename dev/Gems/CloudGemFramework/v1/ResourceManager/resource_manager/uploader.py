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

from uuid import uuid4
from botocore.exceptions import ClientError
from boto3.s3.transfer import S3Transfer
from errors import HandledError

import os
import imp
import mimetypes
import copy
import time

import common_code
from resource_manager_common import constant
import file_util

class Phase():
    PRE_UPDATE=0
    POST_UPDATE=1

class Uploader(object):

    '''Supports uploading data to the project's configuration bucket.'''

    def __init__(self, context, bucket=None, key=None):

        '''Initializes an Uploader object.

        Args:

            context: The Context object used by the uploader.

            bucket (named): The name of the S3 bucket to which content is
            uploaded. Defaults to project's Configuration bucket as found
            using the provided context.

            key (name): The prefix used for the S3 object names used for
            uploaded content. Defaults to 'upload/{unique}' where {unique}
            is a random value.

        '''

        if bucket is None:        
            bucket = context.config.configuration_bucket_name            

        if key is None:
            key = 'upload/{}'.format(uuid4())      

        self.__context = context
        self._bucket = bucket
        self._key = key      

    @property
    def context(self):
        '''The Context object for the uploader.'''
        return self.__context

    @property
    def bucket(self):
        '''The name of the S3 bucket to which content will be uploaded.'''
        return self._bucket

    @bucket.setter
    def bucket(self, value):
        '''The setter for the name of the S3 bucket to which content will be uploaded.'''
        self._bucket = value

    @property
    def key(self):
        '''The object name prefix that will be used when naming uploaded content.'''
        return self._key 


    @staticmethod
    def _get_lambda_function_code_path(context, function_name, source_directory_paths):
        '''Get the directory where the code for the specified lambda function is located.

        Lambda function code is taken from the following locations. All but the first one is deprecated:

            - {source-directory}/lambda-code/{function-name}
            - {source-directory}/{function-name}-lambda-code
            - {source-directory}/lambda-function-code
        
        Arguments:

            context: a Context object.

            source_directory_paths: a list of paths to the directories to search.

            function_name: the name of the lambda function resource.

        Returns the full path to the lambda function's code directory.
        '''

        searched_paths = []

        for source_directory_path in source_directory_paths:

            code_path = os.path.join(source_directory_path, constant.LAMBDA_CODE_DIRECTORY_NAME, function_name)
            if os.path.isdir(code_path):
                return code_path

            searched_paths.append(code_path)

            prefered_path = code_path

            # TODO The use of Name-lambda-code and lambda-function-code directories was deprecated in Lumberyard 1.10.

            code_path = os.path.join(source_directory_path, '{}-lambda-code'.format(function_name))
            if os.path.isdir(code_path):
                context.view.using_deprecated_lambda_code_path(function_name, code_path, prefered_path)
                return code_path

            searched_paths.append(code_path)

            code_path = os.path.join(source_directory_path, 'lambda-function-code'.format(function_name))
            if os.path.isdir(code_path):
                context.view.using_deprecated_lambda_code_path(function_name, code_path, prefered_path)
                return code_path

            searched_paths.append(code_path)

            # End deprecated code.

        raise HandledError('No code was found for the {} AWS Lambda Function. Expected to find one of these directories: {}.'.format(function_name, ', '.join(searched_paths)))


    @staticmethod
    def _get_lambda_function_code_paths(context, function_name, source_directory_paths):
        '''Gets the path to the lambda code directory and paths all to the directories identified 
        by the .import file found in the lambda code directory.

        The lambda code directory is determined using get_lambda_function_code_path.

        Arguments:

            context: a Context object.

            source_directory_paths: a list of paths to the directories to search.

            function_name: the name of the lambda function resource.

        Returns three values:

           code_path: the value returned by get_lambda_function_code_path.
           imported_paths: a list containing paths imported by code_path.
           multi_imports: a dictionary of import names to the set of gem names that will participate in the import.
                          The multi-imports come from .import files containing *.ImportName entries.
        '''
        code_path = Uploader._get_lambda_function_code_path(context, function_name, source_directory_paths)

        imported_paths, multi_imports = common_code.resolve_imports(context, code_path)

        # TODO: The use of shared-lambda-code was deprecated in Lumberyard 1.10.
        for source_directory_path in source_directory_paths:
            shared_lambda_code_path = os.path.join(source_directory_path, 'shared-lambda-code')
            if os.path.isdir(shared_lambda_code_path):
                context.view.using_deprecated_lambda_code_path(function_name, shared_lambda_code_path, os.path.join(source_directory_path, 'common-code', 'LambdaCommon'))
                imported_paths.add(shared_lambda_code_path)
        # End deprecated code
        return code_path, imported_paths, multi_imports


    def zip_and_upload_lambda_function_code(self, function_name, aggregated_directories = None, aggregated_content = None, source_gem=None, keep = False):
        '''Zips and uploads all of the specified lambda function's code from the uploader's source directory.

        The get_lambda_function_code_paths function is used to locate the lambda function's code. The zip will 
        include all the content from common-code directories identified by the .import file found in the lambda
        code directory.

        The content is uploaded to a file named {function-name}-lambda-code.zip.

        Arguments:

            function_name: the name of the function
            
            aggregated_content (named): Additional content included in the zip file. See the docs for
            zip_and_upload_directory.

            aggregated_directories (named): Identifies additional directories included in the zip file.
            See the docs for zip_and_upload_directory.

            keep (named): keep the zip file after uploading. By default the zip file is deleted.

        Returns: the key of the object uploaded.
        '''

        self.context.view.processing_lambda_code(self._source_description, function_name)

        source_directory_paths = self.source_directory_paths
        if source_gem:
            source_directory_paths.append(os.path.join(source_gem.aws_directory_path, 'project-code'))

        code_path, imported_directory_paths, multi_imports = Uploader._get_lambda_function_code_paths(self.context, function_name, source_directory_paths)

        if imported_directory_paths:
            aggregated_directories = copy.deepcopy(aggregated_directories) if aggregated_directories else {}
            value = aggregated_directories.get('', [])
            if not isinstance(value, list):
                value = [ value ] # make sure value is a list
            value.extend(imported_directory_paths)
            aggregated_directories[''] = value

        # Create a top level module for each multi-import module to help discovery of what was imported.
        # This should be the Lambda zip file equivalent of how MultiImportModuleLoader in hook.py loads multi-imports in a local workspace.
        for import_package_name, imported_gem_names in multi_imports.iteritems():
            sorted_gem_names = sorted(imported_gem_names)
            top_level_module_names = { gem_name: '{}__{}'.format(import_package_name, gem_name) for gem_name in sorted_gem_names }

            # Import each top level module ( MyModule__CloudGemName ) using the gem name as the sub-module name ( MyModule.CloudGemName ).
            init_content = [ 'import {} as {}'.format(top_level_module_names[gem_name], gem_name) for gem_name in sorted_gem_names ]

            # Define __all__ for "from package import *".
            init_content.append('__all__ = [{}]'.format(','.join([ '"{}"'.format(gem_name) for gem_name in sorted_gem_names ])))

            # A dictionary of gem name to loaded module for easy iterating ( imported_modules = { CloudGemName: <the_loaded_CloudGemName_module> } ).
            init_content.append('imported_modules = {{{}}}'.format(','.join([ '"{}":{}'.format(gem_name, gem_name) for gem_name in sorted_gem_names ])))

            if aggregated_content is None:
                aggregated_content = {}

            aggregated_content['{}/__init__.py'.format(import_package_name)] = '\n'.join(init_content)

        return self.zip_and_upload_directory(
            code_path, 
            aggregated_directories = aggregated_directories, 
            aggregated_content = aggregated_content,
            file_name = '{}-lambda-code.zip'.format(function_name),
            keep = keep)


    def upload_content(self, name, content, description):
        '''Creates an object in an S3 bucket using the specified content.
        
        Args:

            name: Appended to the uploader's key and used as the object's
            name in the S3 bucket. A '/' will be inserted between the key
            and the specified name.

            content: The content to upload.
            
            description: A description of the content displayed to the user
            during the upload.

        Returns:

            A string containing the URL of the uploaded content.
         
        '''

        bucket = self.bucket
        key = self.key + '/' + name if self.key else name

        self.context.view.uploading_content(bucket, key, description)

        s3 = self.context.aws.client('s3')

        try:
            res = s3.put_object(
                Bucket = bucket,
                Key = key,
                Body = content
            )
        except Exception as e:
            raise HandledError(
                'Cloud not upload {} to S3 bucket {}.'.format(
                    key,
                    bucket 
                ),
                e
            )

        return self.__make_s3_url(bucket, key)

    def upload_file(self, name, path, extra_args = None):
        '''Creates an object in an S3 bucket using content from the specified file.
        
        Args:

            name: Appended to the uploader's key and used as the object's
            name in the S3 bucket. A '/' will be inserted between the key
            and the specified name.

            path: The path to the file to upload.

        Returns:

            A string containing the URL of the uploaded file.
         
        '''
        key = self.__upload_file(name, path, extra_args = extra_args)
        return self.__make_s3_url(self.bucket, key)


    def __upload_file(self, name, path, extra_args = None):
        '''Creates an object in an S3 bucket using content from the specified file.
        
        Args:

            name: Appended to the uploader's key and used as the object's
            name in the S3 bucket. A '/' will be inserted between the key
            and the specified name.

            path: The path to the file to upload.

        Returns:

            A the key of the uploaded file.
         
        '''

        bucket = self.bucket
        key = self.key + '/' + name if self.key else name

        self.context.view.uploading_file(bucket, key, path)

        s3 = self.context.aws.client('s3')
        transfer = S3Transfer(s3)

        try:
            transfer.upload_file(path, bucket, key,extra_args=extra_args if extra_args else {})
        except Exception as e:
            raise HandledError(
                'Cloud not upload {} to S3 bucket {} as {}.'.format(
                    path,
                    bucket,
                    key
                ),
                e
            )

        return key

    def upload_dir(self, name, path, extraargs = None, alternate_root = None, suffix = None):
        '''Creates an object in an S3 bucket using content from the specified directory.
        
        Args:

            name: Appended to the uploader's key and used as the object's
            name in the S3 bucket. A '/' will be inserted between the key
            and the specified name. Name can be None in which case the content
            of the directory will be uploaded directly under the current key.

            path: The path to the file to upload.

            alternate_root (named): if not None (the default), replaces the 
            upload/<uuid> part of the key.

        Returns:

            A string containing the URL of the uploaded file.
         
        '''
        
        if not os.path.isdir(path):
            raise HandledError("The requested path \'{}\' is not a directory.".format(path))

        bucket = self.bucket
        key = self.key if name is None else self.key + '/' + name if self.key else name

        if alternate_root is not None:
             # expecting upload/uuid or upload/uuid/more/stuff/we/keep
            i = key.find('/')
            if i != -1:
                i = key.find('/', i+1)
            if i == -1:
                key = alternate_root
            else:
                key = alternate_root + key[i:]
        if suffix is not None: 
            key = '{}/{}'.format(key, suffix)
        self.context.view.uploading_file(bucket, key, path)        
        s3 = self.context.aws.client('s3')
    
        for root, dirs, files in os.walk(path):          
          for filename in files:            
            file_path = root  + '/' +  filename
            relative_path = os.path.relpath(file_path, path)
            s3_path = key + '/' +  relative_path
            #AWS S3 requires paths to be split by /
            s3_path = s3_path.replace("\\","/")            
            mime_type = mimetypes.guess_type(file_path)
            mime = mime_type[0]
            s3_extraargs = ({'ContentType':mime} if mime else {})
            if extraargs:
                s3_extraargs.update(extraargs)
            try:                              
                self.context.view.uploading_file(bucket, s3_path, relative_path)        
                s3.upload_file(file_path,bucket,s3_path,s3_extraargs)
            except Exception as e:
                raise HandledError(
                      'Cloud not upload {} to S3 bucket {} as {}.'.format(
                       path,
                       bucket,
                       key
                    ),
                    e
                 )
                

        return self.__make_s3_url(bucket, key)


    def __make_s3_url(self, bucket, key):
        return 'https://{bucket}.s3.amazonaws.com/{key}'.format(bucket=bucket, key=key)


    def zip_and_upload_directory(self, directory_path, file_name = None, aggregated_directories = None, aggregated_content = None, extra_args = None, keep = False):
        '''Creates an object in an S3 bucket using a zip file created from the contents of the specified directory.

        Args:

            directory_path: The path to the directory to upload. The object's name in the
            S3 bucket will be the uploader's key with '/', the last element of the path, 
            and '.zip' appended. 

            aggregated_directories (named): a dictionary where the values are additional
            directories whose contents will be included in the zip file, as either a single
            directory path string or a list of directory path strings. The keys are
            the path in the zip file where the content is put. By default no additional
            directories are included.

            aggregated_content (named): a dictionary where the values are additional content
            to include in the zip file. The keys are the path in the zip file where the
            content is put. By default no additional content is included.

            file_name (named): the name of the zip file. By default the directory name with
            '.zip' appended is used.

            keep (named): keep the zip file after uploading. By default the zip file is deleted.

        Returns:

            The key of the uploaded zip file.
         
        Note:

            The directory may contain a .ignore file. If it does, the file should contain
            the names of the files in the directory that will not be included in the zip. 

        '''

        zip_file_path = file_util.zip_directory(
            self.context, 
            directory_path, 
            aggregated_content = aggregated_content, 
            aggregated_directories = aggregated_directories)

        try:

            file_name = file_name or os.path.basename(directory_path) + '.zip'
            return self.__upload_file(file_name, zip_file_path, extra_args=extra_args) 

        finally:
            if os.path.exists(zip_file_path) and not keep:
                self.context.view.deleting_file(zip_file_path)
                os.remove(zip_file_path)

    # Deprecated in 1.9. TODO remove
    def _execute_uploader_hooks(self, handler_name):
        module_name = os.path.join('cli-plugin-code', 'upload.py')
        self.context.hooks.call_module_handlers(module_name, handler_name, args=[self], deprecated=True)


class ProjectUploader(Uploader):

    '''Supports the uploading of project global data to the project's configuration bucket.'''
    
    def __init__(self, context):
        '''Initializes a ProjectUploader object.

        Args:

            context: The Context object used by the uploader.

        '''
        Uploader.__init__(self, context)
        

    @staticmethod
    def __get_source_directory_paths(context):
        return [ context.config.framework_aws_directory_path, context.config.aws_directory_path ]


    @staticmethod
    def get_lambda_function_code_path(context, function_name):
        return Uploader._get_lambda_function_code_path(context, function_name, ProjectUploader.__get_source_directory_paths(context))


    @staticmethod
    def get_lambda_function_code_paths(context, function_name):
        return Uploader._get_lambda_function_code_paths(context, function_name, ProjectUploader.__get_source_directory_paths(context))


    @property
    def source_directory_paths(self):
        return ProjectUploader.__get_source_directory_paths(self.context)


    def get_deployment_uploader(self, deployment_name):
        '''Returns an uploader for deployment specific data.

        Args:

            deployment_name: The name of the deployment.

        Returns:

            A DeploymentUploader object.
            
        '''
        return DeploymentUploader(self, deployment_name)

    # Deprecated in 1.9. TODO remove
    def execute_uploader_pre_hooks(self):
        '''Calls the upload_project_content_pre methods defined by uploader hook modules for uploading before the project stack has been updated.

        The methods are called with this uploader as their only parameter.
        
        Uploader hook modules are defined by upload.py files in the project's AWS directory 
        and in resource group directories.'''

        self._execute_uploader_hooks('upload_project_content_pre')

    # Deprecated in 1.9. TODO remove
    def execute_uploader_post_hooks(self):
        '''Calls the upload_project_content_post methods defined by uploader hook modules for uploading after the project stack has been updated.

        The methods are called with this uploader as their only parameter.
        
        Uploader hook modules are defined by upload.py files in the project's AWS directory 
        and in resource group directories.'''

        self._execute_uploader_hooks('upload_project_content_post')
    
    @property
    def _source_description(self):
        return "project's"

class DeploymentUploader(Uploader):
    
    '''Supports the uploading of deployment specific data to the project's configuration bucket.'''
    
    def __init__(self, project_uploader, deployment_name):
        '''Initializes a DeploymentUploader object.

        Args:

            project_uploader: The ProjectUploader object on which the uploader is based.

            deployment_name: The name of the deployment targed by the uploader.

        '''

        Uploader.__init__(
            self, 
            project_uploader.context, 
            project_uploader.bucket, 
            project_uploader.key + '/deployment/' + deployment_name)

        self._project_uploader = project_uploader
        self._deployment_name = deployment_name
        self._resource_group_uploaders = {}


    @staticmethod
    def __get_source_directory_paths(context):
        return [ context.config.framework_aws_directory_path, context.config.aws_directory_path ]


    @staticmethod
    def get_lambda_function_code_path(context, function_name):
        return Uploader._get_lambda_function_code_path(context, function_name, DeploymentUploader.__get_source_directory_paths(context))


    @staticmethod
    def get_lambda_function_code_paths(context, function_name):
        return Uploader._get_lambda_function_code_paths(context, function_name, DeploymentUploader.__get_source_directory_paths(context))


    @property
    def source_directory_paths(self):
        return DeploymentUploader.__get_source_directory_paths(self.context)


    @property
    def project_uploader(self):
        '''The ProjectUploader object provided when this object was created'''
        return self._project_uploader

    @property
    def deployment_name(self):
        '''The name of the deployment targeted by ths uploader.'''
        return self._deployment_name
        
    def get_resource_group_uploader(self, resource_group_name):
        '''Returns an uploader for resource group specific data.

        Args:

            resource_group_name: The name of the resource group.

        Returns:

            A ResourceGroupUploader object.
            
        '''
        
        if self._resource_group_uploaders.has_key(resource_group_name):
            return self._resource_group_uploaders[resource_group_name]
        
        rgu = ResourceGroupUploader(self, resource_group_name)
        self._resource_group_uploaders[resource_group_name] = rgu

        return rgu

    # Deprecated in 1.9. TODO remove
    def execute_uploader_pre_hooks(self):
        '''Calls the upload_deployment_pre_content methods defined by uploader hook modules.

        The methods are called with this uploader as their only parameter.
        
        Uploader hook modules are defined by upload.py files in the project's AWS directory 
        and in resource group directories.'''

        self._execute_uploader_hooks('upload_deployment_content_pre')    
        
    # Deprecated in 1.9. TODO remove
    def execute_uploader_post_hooks(self):
        '''Calls the upload_deployment_post_content methods defined by uploader hook modules.

        The methods are called with this uploader as their only parameter.
        
        Uploader hook modules are defined by upload.py files in the project's AWS directory 
        and in resource group directories.'''

        self._execute_uploader_hooks('upload_deployment_content_post')

    @property
    def _source_description(self):
        return "deployment's"


class ResourceGroupUploader(Uploader):
    '''Supports the uploading of deployment specific data to the project's configuration bucket.'''
    
    def __init__(self, deployment_uploader, resource_group_name):
        '''Initializes a ResourceGroupUploader object.

        Args:

            deployment_uploader: The DeploymentUploader object on which the uploader is based.

            resource_group_name: The name of the resource group targeted by the uploader.

        '''

        Uploader.__init__(
            self, 
            deployment_uploader.context, 
            deployment_uploader.bucket, 
            deployment_uploader.key + '/resource-group/' + resource_group_name)

        self._deployment_uploader = deployment_uploader
        self._resource_group_name = resource_group_name

    @staticmethod
    def __get_source_directory_paths(context, resource_group_name):
        resource_group = context.resource_groups.get(resource_group_name)
        return [ resource_group.directory_path ]

    @staticmethod
    def get_lambda_function_code_path(context, resource_group_name, function_name):
        try:
            return Uploader._get_lambda_function_code_path(context, function_name, ResourceGroupUploader.__get_source_directory_paths(context, resource_group_name))
        except HandledError as e:
            error_message = e.msg
            error_message = error_message + '\n\n'
            error_message = error_message + "You can create a new folder with the default CloudCanvas Lambda code by running the following command: lmbr_aws function create-folder -r {} -f {}".format(resource_group_name, function_name)
            raise HandledError(error_message)

    @staticmethod
    def get_lambda_function_code_paths(context, resource_group_name, function_name):
        try:
            return Uploader._get_lambda_function_code_paths(context, function_name, ResourceGroupUploader.__get_source_directory_paths(context, resource_group_name))
        except HandledError as e:
            error_message = e.msg
            error_message = error_message + '\n\n'
            error_message = error_message + "You can create a new folder with the default CloudCanvas Lambda code by running the following command: lmbr_aws function create-folder -r {} -f {}".format(resource_group_name, function_name)
            raise HandledError(error_message)

    @property
    def source_directory_paths(self):
        return ResourceGroupUploader.__get_source_directory_paths(self.context, self.resource_group_name)


    @property
    def deployment_uploader(self):
        '''The DeploymentUploader provided when this object was created.'''
        return self._deployment_uploader

    @property
    def resource_group_name(self):
        '''The name of the resource group targeted by this uploader.'''
        return self._resource_group_name

    @property
    def resource_group(self):
        '''A ResourceGroup object representing the resource group targeted by this uploader.'''
        return self.context.resource_groups.get(self.resource_group_name)
        
    # Deprecated in 1.9. TODO remove
    def execute_uploader_pre_hooks(self):
        '''Calls the upload_resource_group_pre_content methods defined by uploader hook modules.

        The methods are called with the context, and this uploader as their only parameters.
        
        Uploader hook modules are defined by upload.py files in the project's AWS directory 
        and in resource group directories.'''

        self._execute_uploader_hooks('upload_resource_group_content_pre')        

    # Deprecated in 1.9. TODO remove
    def execute_uploader_post_hooks(self):
        '''Calls the upload_resource_group_post_content methods defined by uploader hook modules.

        The methods are called with the context, and this uploader as their only parameters.
        
        Uploader hook modules are defined by upload.py files in the project's AWS directory 
        and in resource group directories.'''

        self._execute_uploader_hooks('upload_resource_group_content_post')        

    @property
    def _source_description(self):
        return self.resource_group_name + " resource group's"

