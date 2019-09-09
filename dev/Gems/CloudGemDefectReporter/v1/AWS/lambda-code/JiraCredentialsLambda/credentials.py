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
# Function resource definition in the groups's resource-template.json file identifies 
# the Python function that is called when the Lambda Function is execution. To call 
# a function here in main.py, set the Handler property to "main.FUNCTION_NAME".
#
# IMPORTANT: If the game executes the Lambda Function (which is often the case), then
# you must configure player access for the Lambda Function. This is done by including
# the CloudCanvas Permission metadata on the Lambda Function resource definition.
#

import boto3
import os
from base64 import b64encode, b64decode

CREDENTIAL_KEYS = ['userName', 'password', 'server']

def update_jira_credentials(request, context):
    lambda_client = boto3.client('lambda')
    kms_client = boto3.client('kms')

    environment_variables = {}
    for credential_key in CREDENTIAL_KEYS:
        if request.get(credential_key, ''):
            encrypted_value = b64encode(kms_client.encrypt(KeyId = request['kms_key'], Plaintext = request[credential_key])['CiphertextBlob'])
            environment_variables[credential_key] = encrypted_value

    lambda_client.update_function_configuration(
        FunctionName=context.invoked_function_arn,
        Environment={
            'Variables': environment_variables
        }
    )
    return {'status': 'SUCCEED'}

def get_jira_credentials():
    kms_client = boto3.client('kms')
    jira_credentials = {}
    for credential_key in CREDENTIAL_KEYS:
        decrypted_value = kms_client.decrypt(CiphertextBlob=b64decode(os.getenv(credential_key)))['Plaintext'] if os.getenv(credential_key) else ''
        jira_credentials[credential_key] = decrypted_value

    return jira_credentials

def main(request, context):
    if request.get('method', '') == 'PUT':
        return update_jira_credentials(request, context)
    elif request.get('method', '') == 'GET':
        return get_jira_credentials()