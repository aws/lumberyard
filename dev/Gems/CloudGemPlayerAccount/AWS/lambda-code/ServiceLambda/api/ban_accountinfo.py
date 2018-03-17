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

import account_utils
import boto3
import CloudCanvas
import errors
import service
from random import randint
import uuid

@service.api(logging_filter=account_utils.apply_logging_filter)
def get(request, cog_id):
    account = account_utils.get_account_for_identity(cog_id)
    return account
