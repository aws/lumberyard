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

## Project
PROJECT_DEPLOYMENT_TEMPLATE_FILENAME = "deployment-template.json"
PROJECT_GEMS_DEFINITION_FILENAME = "gems.json"
PROJECT_TEMPLATE_FILENAME = "project-template.json"
PROJECT_TEMPLATE_EXTENSIONS_FILENAME = "project-template-extensions.json"
DEPLOYMENT_TEMPLATE_FILENAME = "deployment-template.json"
DEPLOYMENT_TEMPLATE_EXTENSIONS_FILENAME = "deployment-template-extensions.json"
DEPLOYMENT_ACCESS_TEMPLATE_FILENAME = "deployment-access-template.json"
DEPLOYMENT_ACCESS_TEMPLATE_EXTENSIONS_FILENAME = "deployment-access-template-extensions.json"
PROJECT_LOCAL_SETTINGS_FILENAME ="local-project-settings.json"
PROJECT_SETTINGS_FILENAME ="project-settings.json"
PROJECT_CONFIGURATION_RESOURCE_NAME = "Configuration"
PROJECT_RESOURCE_HANDLER_NAME = "ProjectResourceHandler"
PROJECT_CGP_RESOURCE_NAME = "CloudGemPortal"
PROJECT_CGP_ROOT_FOLDER = "www"
PROJECT_CGP_ROOT_FILE = "{}/index.html".format(PROJECT_CGP_ROOT_FOLDER)
PROJECT_CGP_ROOT_APP = "{}/bundles/app.bundle.js".format(PROJECT_CGP_ROOT_FOLDER)
PROJECT_CGP_ROOT_APP_DEPENDENCIES = "{}/bundles/dependencies.bundle.js".format(PROJECT_CGP_ROOT_FOLDER)
PROJECT_CGP_1 = "a"
PROJECT_CGP_2 = "b"
PROJECT_CGP_DEFAULT_EXPIRATION_SECONDS = 604800 # 1 weeks
PROJECT_CGP_AES_SEGMENT_SIZE = 128
PROJECT_CGP_EXPIRATION_OUT_OF_BOUNDS_MESSAGE = 'The --duration-seconds option value must be in the range 900-3600, inclusive.'
STACK_UPDATE_DELAY_TIME = 10 # seconds of delay after uploading a template to s3 before asking cloud formation to use it
MAPPING_FILE_SUFFIX = 'awsLogicalMappings.json'
RESOURCE_DEFINITIONS_PATH = "resource-definitions"
S3_DELIMETER = "/"

## Gem
GEM_DEFINITION_FILENAME = "gem.json"
RESOURCE_GROUP_TEMPLATE_FILENAME = "resource-template.json"
GEM_AWS_DIRECTORY_NAME = "AWS"
GEM_CODE_DIRECTORY_NAME = "Code"
GEM_CGP_DIRECTORY_NAME = "cgp-resource-code"
USER_SETTINGS_FILENAME = "user-settings.json"
AUTH_SETTINGS_FILENAME = "auth-settings.json"
RESOURCE_NAME_VALIDATION_CONFIG_FILENAME = "aws_name_validation_rules.json"
RESOURCE_GROUP_SETTINGS = "resource-group-settings.json"

# Directory Names
## Project
PROJECT_GEMS_DIRECTORY_NAME = "Gems"
PROJECT_CODE_DIRECTORY_NAME = "Code"
PROJECT_CACHE_DIRECTORY_NAME = "Cache"
PROJECT_USER_DIRECTORY_NAME = "User"
PROJECT_AWS_DIRECTORY_NAME = "AWS"
PROJECT_TOOLS_DIRECTORY_NAME = "Tools"
PROJECT_RESOURCE_NAME_IDENTITY_POOL = "ProjectIdentityPool"
PROJECT_RESOURCE_NAME_USER_POOL = "ProjectUserPool"
LAMBDA_CODE_DIRECTORY_NAME = 'lambda-code'
COMMON_CODE_DIRECTORY_NAME = "common-code"
IMPORT_FILE_NAME = ".import"

## CLI
CLI_RETURN_OK_CODE = 0
CLI_RETURN_ERROR_HANDLED_CODE = 1
CLI_RETURN_ERROR_UNHANDLED_CODE = 2

## User
DEFAULT_PROFILE_NAME = "DefaultProfile"
DEFAULT_SECTION_NAME = '__default__'

## Credentials
ACCESS_KEY_OPTION = 'aws_access_key_id'
SECRET_KEY_OPTION = 'aws_secret_access_key'

## Local Project Settings
ENABLED_RESOURCE_GROUPS_KEY = 'EnabledResourceGroups' # deprecated
DISABLED_RESOURCE_GROUPS_KEY = 'DisabledResourceGroups'
SET = 'Set'
DEFAULT = 'default'
LAZY_MIGRATION = 'IsLazyMigration'
PENDING_PROJECT_STACK_ID = 'PendingProjectStackId'
PROJECT_STACK_ID = 'ProjectStackId'
FRAMEWORK_VERSION_KEY = 'FrameworkVersion'

