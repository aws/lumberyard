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

import json
import boto3
from cgf_utils import aws_utils
from cgf_utils import properties

REFERENCE_METADATA_PATH = "reference-metadata"
REFERENCE_TYPE = "Custom::ExternalResourceReference"

s3_client = aws_utils.ClientWrapper(boto3.client('s3'))

def is_reference_type(type):
    return True if type == REFERENCE_TYPE else False

def get_reference_metadata_key(project_name, reference_name):
    return aws_utils.s3_key_join(REFERENCE_METADATA_PATH, project_name, reference_name + ".json")

def get_reference_metadata(configuration_bucket, project_name, reference_name):
    reference_metadata_key = get_reference_metadata_key(project_name, reference_name)

    contents = s3_client.get_object(Bucket=configuration_bucket, Key=reference_metadata_key)['Body'].read()
    if contents is None or len(contents) == 0:
        return {}

    return json.loads(contents)

def load_reference_metadata_properties(event):
    # Strip the fields other than 'ReferenceMetadata' from event['ResourceProperties']
    reference_metadata = event.get('ResourceProperties', {}).get('ReferenceMetadata', {})
    event['ResourceProperties'] = {'ReferenceMetadata': reference_metadata}

    return properties.load(event, {
      'ReferenceMetadata': properties.Object(schema={
        'Arn': properties.String(),
        'PhysicalId': properties.String(),
        'Permissions': properties.Object(schema={
            'Action': properties.StringOrListOfString(),
            'ResourceSuffix': properties.StringOrListOfString()
        })
      })
    })
