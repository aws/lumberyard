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

import boto3
import leaderboard_utils
import CloudCanvas
import stats_settings
import reservoir_sampler

RESERVOIR_SAMPLERS = {}

def get_scores_for_user(user, stats=[]):
    valid_stats = [stat["name"] for stat in stats_settings.get_leaderboard_stats()]
    if not stats: # if no stats are provided just get all
        stats = valid_stats
    elif not set(stats).issubset(valid_stats): # you requested a stat that is not valid
        raise Exception("There is an invalid stat in request.") #TODOL client error

    try:
        high_score_table = boto3.resource('dynamodb').Table(CloudCanvas.get_setting("MainTable"))
    except ValueError as e:
        raise Exception("Error getting table reference")

    table_key = {
        "user" : user
    }
    table_response = high_score_table.get_item(Key=table_key)
    all_player_scores = table_response.get("Item", {})

    if not all_player_scores:
        return []

    score_list = []
    for stat in stats:
        score = leaderboard_utils.generate_score_dict(all_player_scores, stat, user)
        if score:
            score_list.append(score)

    return score_list


def build_leaderboard(stat, users):
    global RESERVOIR_SAMPLERS
    if not stat in RESERVOIR_SAMPLERS:
        RESERVOIR_SAMPLERS[stat] = reservoir_sampler.DynamoDBBackedReservoirSampler(stat)
    sample = RESERVOIR_SAMPLERS[stat].get_sample()
    user_scores = []
    for user in users:
        if not user in [s["user"] for s in sample]:
            user_scores = user_scores + get_scores_for_user(user, [stat])
    return leaderboard_utils.custom_leaderboard(sample,
                                                leaderboard_utils.sort_leaderboard(user_scores, stat),
                                                RESERVOIR_SAMPLERS[stat].get_full_dataset_size(),
                                                stat)
