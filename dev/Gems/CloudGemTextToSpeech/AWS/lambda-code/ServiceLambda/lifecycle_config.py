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


LIFECEYCLE_CONFIG_RULE = {
    "ID": "DisableRuntimeCaching",
    "Status": "Enabled",
    "Expiration": {
        "Days": 1,
    },
    "NoncurrentVersionExpiration": {
        "NoncurrentDays": 1,
    },
    "Filter": {
        "Tag": {
            "Key": "letexpire",
            "Value": "true"
        }
    }
}

import CloudCanvas
import boto3
from botocore.exceptions import ClientError


# This lifecycle rule is not really relevant until they attempt to turn off caching.
# Also CloudFormation does not let you specify rules with Filters, which we need
def set_lifecycle_config():
    config = get_lifecycle_config()
    rules = config.get('Rules', [])
    for rule in rules:
        if rule['ID'] == 'DisableRuntimeCaching':
            return
    rules.append(LIFECEYCLE_CONFIG_RULE)
    if rules:
        config = { "Rules": rules }
        boto3.client("s3").put_bucket_lifecycle_configuration(Bucket=CloudCanvas.get_setting('ttscache'), LifecycleConfiguration=config)


def get_lifecycle_config():
    try:
        lifecycle_config = boto3.client("s3").get_bucket_lifecycle_configuration(Bucket=CloudCanvas.get_setting('ttscache'))
        return lifecycle_config
    except ClientError as e:
        if hasattr(e, 'response') and e.response and e.response['Error']['Code'] == 'NoSuchLifecycleConfiguration':
            print("No lifecycle config, returning empty")
        else:
            raise e
    return {}