#
# IMPORTANT: If the game executes the Lambda Function (which is often the case), then
# you must configure player access for the Lambda Function. This is done by including
# the CloudCanvas Permission metadata on the Lambda Function resource definition.
#
# An example Lambda Function resource definitions that grants player access is shown below:
#
#        "Lambda": {
#           "Type": "AWS::Lambda::Function",
#            "Properties": {
#                "Description": "Example of a function called by the game.",
#                "Handler": "main.handler",
#                "Role": { "Fn::GetAtt": [ "MainConfiguration", "Role" ] },
#                "Runtime": { "Fn::GetAtt": [ "MainConfiguration", "Runtime" ] },
#                "Code": {
#                    "S3Bucket": { "Fn::GetAtt": [ "MainConfiguration", "ConfigurationBucket" ] },
#                    "S3Key": { "Fn::GetAtt": [ "MainConfiguration", "ConfigurationKey" ] }
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
#        }
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

def handler(event, context):
    
    print 'event: {}'.format(event)

    response = {}

    return response
