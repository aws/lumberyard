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

from __future__ import print_function

import os
from botocore.client import Config
import boto3
import defect_reporter_constants as constants


def get_client():
    '''Returns a S3 client with proper configuration.'''

    current_region = os.environ.get('AWS_REGION')

    if current_region is None:
        raise RuntimeError('AWS region is empty')

    configuration = Config(signature_version=constants.S3_SIGNATURE_VERSION)

    client = boto3.client('s3', region_name=current_region, api_version=constants.S3_API_VERSION, config=configuration)

    return client
