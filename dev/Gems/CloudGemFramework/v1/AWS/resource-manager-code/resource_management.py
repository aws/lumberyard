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

import os

from resource_manager.errors import HandledError

LAMBDA_CONFIGURATION_RESOURCE_NAME      = 'ServiceLambdaConfiguration'
LAMBDA_FUNCTION_RESOURCE_NAME           = 'ServiceLambda'
API_RESOURCE_NAME                       = 'ServiceApi'
SERVICE_URL_OUTPUT_NAME                 = 'ServiceUrl'

CACHE_CLUSTER_ENABLED_PARAMETER_NAME    = 'ServiceApiCacheClusterEnabled'
CACHE_CLUSTER_SIZE_PARAMETER_NAME       = 'ServiceApiCacheClusterSize'

RESOURCE_DEFINITIONS = {

    LAMBDA_CONFIGURATION_RESOURCE_NAME: {
        "Type": "Custom::LambdaConfiguration",
        "Properties": {
            "ServiceToken": { "Ref": "ProjectResourceHandler" },
            "ConfigurationBucket": { "Ref": "ConfigurationBucket" },
            "ConfigurationKey": { "Ref": "ConfigurationKey" },
            "Runtime": "python2.7",
            "FunctionName": LAMBDA_FUNCTION_RESOURCE_NAME,
            "Settings": {
            }
        }
    },

    LAMBDA_FUNCTION_RESOURCE_NAME: {
      "Type" : "AWS::Lambda::Function",
      "Properties" : {
        "Code" : {
            "S3Bucket" : { "Fn::GetAtt": [ LAMBDA_CONFIGURATION_RESOURCE_NAME, "ConfigurationBucket" ] },
            "S3Key" : { "Fn::GetAtt": [ LAMBDA_CONFIGURATION_RESOURCE_NAME, "ConfigurationKey" ] },
        },
        "Handler" : "service.dispatch",
        "Role" : { "Fn::GetAtt": [ LAMBDA_CONFIGURATION_RESOURCE_NAME, "Role" ] },
        "Runtime" : { "Fn::GetAtt": [ LAMBDA_CONFIGURATION_RESOURCE_NAME, "Runtime" ] }
      },
      "Metadata": {
        "CloudCanvas": {
            "Permissions": [
                {
                    "AbstractRole": "ServiceApi",
                    "Action": "lambda:InvokeFunction"
                }
            ]
        }
      }
    },

    API_RESOURCE_NAME: {
        "Type": "Custom::ServiceApi",
        "Properties": {
            "ServiceToken": { "Ref": "ProjectResourceHandler" },
            "ConfigurationBucket": { "Ref": "ConfigurationBucket" },
            "ConfigurationKey": { "Ref": "ConfigurationKey" },
            "SwaggerSettings": {
                "ServiceLambdaArn": { 
                    "Fn::GetAtt": [ 
                        LAMBDA_FUNCTION_RESOURCE_NAME, 
                        "Arn" 
                    ]
                }
            },
            "CacheClusterEnabled" : { "Ref": CACHE_CLUSTER_ENABLED_PARAMETER_NAME },
            "CacheClusterSize" : { "Ref": CACHE_CLUSTER_SIZE_PARAMETER_NAME },
            "MethodSettings" : {}
        }
    }

}

RESOURCE_DEPENDENCIES = {
    'AccessControl': [ API_RESOURCE_NAME, LAMBDA_FUNCTION_RESOURCE_NAME ]
}

PARAMETER_DEFINITIONS = {

    CACHE_CLUSTER_ENABLED_PARAMETER_NAME: {
        "Type": "String", 
        "Description": "Indicates whether cache clustering is enabled for the service API.",
        "Default": "false"
    },

    CACHE_CLUSTER_SIZE_PARAMETER_NAME: {
        "Type": "String", 
        "Description": "Indicates whether cache clustering is enabled for the service API.",
        "Default": "0.5"
    }

}


def add_resources(context, args):

    resource_group = context.resource_groups.get(args.resource_group)

    source_path = os.path.join(os.path.dirname(__file__), 'default-resource-group-content')
    resource_group.copy_directory(source_path, force = args.force)

    resource_group.add_resources(RESOURCE_DEFINITIONS, force = args.force, dependencies = RESOURCE_DEPENDENCIES)
    resource_group.add_parameters(PARAMETER_DEFINITIONS, force = args.force)
    resource_group.add_output(SERVICE_URL_OUTPUT_NAME, 'The service url.', { "Fn::GetAtt": [ API_RESOURCE_NAME, "Url" ] } )

    resource_group.save_template()


def remove_resources(context, args):
    resource_group = context.resource_groups.get(args.resource_group)
    resource_group.remove_resources(RESOURCE_DEFINITIONS.keys())
    resource_group.remove_parameters(PARAMETER_DEFINITIONS.keys())
    resource_group.remove_output(SERVICE_URL_OUTPUT_NAME)
    resource_group.save_template()


def add_cli_commands(subparsers, addCommonArgs):

    # add-service-api-resources
    subparser = subparsers.add_parser('add-service-api-resources', help='Add the resources necessery to implement a swagger.json based service API to a resource group.')
    subparser.add_argument('--resource-group', required=True, metavar='GROUP', help='The name of the resource group.')
    subparser.add_argument('--force', required=False, action='store_true', help='Force the replacement of existing resource defintions. By default existing resource definitions with the same names are not changed.')
    addCommonArgs(subparser)
    subparser.set_defaults(func=add_resources)

    # remove-service-api-resources
    subparser = subparsers.add_parser('remove-service-api-resources', help='Remove the resources that implement a swagger.json based service API from a resource group.')
    subparser.add_argument('--resource-group', required=True, metavar='GROUP', help='The name of the resource group.')
    addCommonArgs(subparser)
    subparser.set_defaults(func=remove_resources)

