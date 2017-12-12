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

import service
import boto3
import survey_common
import errors
from botocore.exceptions import ClientError
import json

@service.api
def get(request, survey_id, question_index=None, question_count=None):
    return survey_common.get_survey(survey_id, question_index, question_count, True, False)
