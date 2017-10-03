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

#
# If the resource group defines one or more AWS Lambda Function resources, you can put 
# the code that implements the functions below. The Handler property of the Lambda 
# Function resource definition in the groups's resource-template.json file identifies 
# the Python function that is called when the Lambda Function is execution. To call 
# a function here in main.py, set the Handler property to "main.FUNCTION_NAME".
#
# IMPORTANT: If the game executes the Lambda Function (which is often the case), then
# you must configure player access for the Lambda Function. This is done by including
# the CloudCanvas Permission metadata on the Lambda Function resource definition.
#
# An example Lambda Function resource definitions that grants player access is shown below:
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
#                    "Permissions": {
#                        "AbstractRole": "Player",
#                        "Action": "lambda:InvokeFunction"
#                    }
#                }
#            }
#        },
#

import boto3            # Python AWS API
import CloudCanvas      # Access Settings using get_setting("SETTING_NAME")

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

messages_table_name = CloudCanvas.get_setting('MessagesTable')
greeting = CloudCanvas.get_setting('Greeting')

messages_table = boto3.resource('dynamodb').Table(messages_table_name)

def say_hello(event, context):
    
    print 'event: {}'.format(event)
    print 'context {}'.format(context)
    print 'messages_table_name: "{}"'.format(messages_table_name)
    print 'greeting: "{}"'.format(greeting)

    target = event['Target']

    message = greeting + ' ' + target

    response = messages_table.put_item(
        Item={
            'PlayerId': context.identity.cognito_identity_id,
            'Message': message
        })

    return message
