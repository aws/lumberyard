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
import survey_common
import survey_utils
import validation_utils
import validation_common
import errors
import json
import time
import StringIO
import uuid

@service.api
def put(request, survey_id, update_activation_period_input):
    validation_common.validate_survey_id(survey_id)
    activation_start_time, activation_end_time = validation_common.validate_activation_period(
        update_activation_period_input.get('activation_start_time'), update_activation_period_input.get('activation_end_time'))

    survey_common.ensure_survey_exists(survey_id)

    expression_attribute_values = {':activation_start_time':activation_start_time}
    update_expression = 'SET activation_start_time = :activation_start_time'
    if activation_end_time is None:
        update_expression += ' REMOVE activation_end_time'
    else:
        update_expression += ',activation_end_time = :activation_end_time'
        expression_attribute_values[':activation_end_time'] = activation_end_time

    survey_utils.get_survey_table().update_item(
        Key={'survey_id':survey_id},
        UpdateExpression=update_expression,
        ExpressionAttributeValues=expression_attribute_values
    )

    return "success"
