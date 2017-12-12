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
import CloudCanvas
import survey_utils
import survey_common
import validation_utils
import validation_common
from boto3.dynamodb.conditions import Key
import errors

@service.api
def get(request, survey_id, submission_id):
    submission = survey_common.get_answer_submission(survey_id, submission_id)
    if submission is None:
        raise errors.ClientError('No survey found for survey_id: [{}], submission_id: [{}]'.format(survey_id, submission_id))

    user_id = submission['user_id']

    if user_id != request.event["cognitoIdentityId"]:
        raise errors.ForbiddenRequestError("Player didn't submit this submission")

    del submission['user_id']

    return submission
