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
# $Revision: #9 $

#
# If the resource group defines one or more AWS Lambda Function resources, you can put 
# the code that implements the functions below. The Handler property of the Lambda 
# Function resoruce definition in the groups's resource-template.json file identifies 
# the Python function that is called when the Lambda Function is execution. To call 
# a function here in main.py, set the Handler property to "main.FUNCTION_NAME".
#
# IMPORTANT: If the game executes the Lambda Function (which is often the case), then
# you must configure player access for the Lambda Function. This is done by including
# the CloudCanvas PlayerAccess metadata on the Lambda Function resource definition.
# You must also modify the PlayerAccess custom resource definition so that it depends
# on the Lambda Function resource.
#
# An example Lambda Function and PlayerAccess resource definitions are shown below:
#
#        "SayHello": {
#           "Type": "AWS::Lambda::Function",
#            "Properties": {
#                "Description": "Example of a function called by the game to write data into a DynamoDB table.",
#                "Handler": "main.say_hello",
#                "Role": { "Fn::GetAtt": [ "SayHelloConfiguration", "Role" ] },
#                "Runtime": { "Fn::GetAtt": [ "SayHelloConfiguration", "Runtime" ] },
#                "Code": {
#                    "S3Bucket": { "Fn::GetAtt": [ "SayHelloConfiguration", "ConfigurationBucket" ] },
#                    "S3Key": { "Fn::GetAtt": [ "SayHelloConfiguration", "ConfigurationKey" ] }
#                }
#            },
#            "Metadata": {
#                "CloudCanvas": {
#                    "PlayerAccess": {
#                        "Action": "lambda:InvokeFunction"
#                    }
#                }
#            }
#        },
#
#        "PlayerAccess": {
#            "Type": "Custom::PlayerAccess",
#            "Properties": {
#                "ServiceToken": { "Ref": "ProjectResourceHandler" },
#                "ConfigurationBucket": { "Ref": "ConfigurationBucket" },
#                "ConfigurationKey": { "Ref": "ConfigurationKey" },
#                "ResourceGroupStack": { "Ref": "AWS::StackId" }
#            },
#            "DependsOn": [ "SayHello" ]
#        }
#

import boto3            # Python AWS API

# Setting values come from the Settings property of the AWS Lambda Function's 
# configuration resource definition in the resource groups's resource-template.json 
# file. 
#
# You can use settings to pass resource template parameter values to the Lambda
# Function. The template parameter's default values can be overriden using the 
# project project-settings.json file. You can provide different parameter values 
# for each deployment.
#
# You can also use settings to pass a resource's physical id to the Lambda Function.
# The Lambda Function code can use the pyhsical id to access the AWS resource using
# the boto3 api.


def AWSBehaviorLambdaExample(event, context):
    return 'Hello World'