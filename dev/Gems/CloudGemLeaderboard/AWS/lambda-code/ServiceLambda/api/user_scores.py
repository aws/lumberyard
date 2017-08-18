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
import score_reader
import score_submitter

@service.api
def get(request, user = None, stat = None):
    final_list = score_reader.get_scores_for_user(user)
    if stat:
        final_list = [score for score in final_list if score['stat'] == stat]
    return {
        "scores": final_list
        }


@service.api
def delete(request, user = None, stat = None):
    score_submitter.delete_score(user, stat)
    return {
        "scores": score_reader.get_scores_for_user(user)
        }
