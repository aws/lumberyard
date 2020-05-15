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
import errors
import survey_utils
import survey_common
import validation_utils
import validation_common

MAX_LIMIT = 1000

@service.api
def get(request, survey_name=None, limit=None, pagination_token=None, sort=None):
    if validation_utils.is_empty(survey_name):
        return survey_common.get_survey_metadata_list(limit, MAX_LIMIT, pagination_token, sort, False)
    else:
        return survey_common.get_survey_metadata_by_survey_name(survey_name, limit, pagination_token, sort)
