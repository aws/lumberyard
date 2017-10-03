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
import validation_utils
import validation_common
import survey_utils
import survey_common
import errors

@service.api
def put(request, survey_id, survey_published_status):
    validation_common.validate_survey_id(survey_id)
    published = survey_published_status.get('published')
    validation_utils.validate_param(published, 'published', validation_utils.is_not_none)

    if published:
        survey_metadata = survey_utils.get_survey_metadata_by_id(
            survey_id, ['questions', 'activation_start_time', 'activation_end_time'])

        validate_eligible_for_publishing(survey_metadata)

        survey_utils.get_survey_table().update_item(
            Key={'survey_id':survey_id},
            UpdateExpression='SET published = :true',
            ExpressionAttributeValues={':true':1}
        )
    else:
        survey_utils.get_survey_table().update_item(
            Key={'survey_id':survey_id},
            UpdateExpression='REMOVE published'
        )

    return 'success'

def validate_eligible_for_publishing(survey_metadata):
    questions = survey_metadata.get('questions')
    if validation_utils.is_empty(questions):
        raise errors.ClientError("Survey doesn't have question")

    if survey_metadata.get('activation_start_time') is None and survey_metadata.get('activation_end_time') is None:
        raise errors.ClientError("Survey doesn't have activation period yet")
