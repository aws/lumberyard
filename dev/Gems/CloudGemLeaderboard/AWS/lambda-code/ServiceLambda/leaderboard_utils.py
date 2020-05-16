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

import ban_handler
import stats_settings

'''
Validates whether the given score is allowed for that stat
Insert your own validation logic here
'''
def is_score_valid(score, stat):
    ok = True
    ok = ok and isinstance(score, (int, float))
    settings = stats_settings.get_stat(stat)
    if "min" in settings:
        ok = ok and float(score) >= float(settings["min"])
    if "max" in settings:
        ok = ok and float(score) <= float(settings["max"])
    return ok

'''
Validates user for a given stat
Insert your own validation logic here
'''
def is_user_valid(user):
    return not ban_handler.is_player_banned(user)

'''
Resolves updates for a given stat
Reads score config file if available.
If no config is found, defaults to WriteMode = Overwrite, ScoreComparison = >
Returns the entry to be written to the table
'''
def resolve_update(submission, existing_data):
    if not existing_data.get(submission["stat"], False):
        existing_data[submission["stat"]] = submission["value"]
        return existing_data
    settings = stats_settings.get_stat(submission["stat"])
    mode = settings.get("mode", "OVERWRITE")
    if mode == "OVERWRITE" and submission["value"] > existing_data[submission["stat"]]:
        existing_data[submission["stat"]] = submission["value"]
    elif mode == "INCREMENT":
        existing_data[submission["stat"]] = submission["value"] + existing_data[submission["stat"]]
    return existing_data

'''
puts score in sorted_scores depending on the config of stat
'''
def __insert_score(sorted_scores, score, stat):
    if not sorted_scores:
        return [score]
    for i in range(0, len(sorted_scores)):
        if score["value"] > sorted_scores[i]["value"]:
            sorted_scores.insert(i, score)
            return sorted_scores
    sorted_scores.append(score)
    return sorted_scores


'''
Sorts the leaderboard given a list of scores and a stat
TODO: make the comparison configurable
'''
def sort_leaderboard(scores, stat):
    if not scores:
        return []
    sorted_scores = []
    for score in scores:
        sorted_scores = __insert_score(sorted_scores, score, stat)
    return sorted_scores

'''
Given a MainTable entry, generate a score dict for just the stat you are interested in
'''
def generate_score_dict(table_entry, stat, user = None):
    score = table_entry.get(stat, None)
    if not user:
        user = table_entry["user"]
    if not score:
        return None
    return {
        "user": user,
        "stat": stat,
        "value": score
    }

'''
Given a sorted sample and a sorted list of user scores make a leaderboard
'''
def custom_leaderboard(sample, user_scores, stat_population, stat):
    sample_size = len(sample)
    sample = estimate_sample_ranks(sample, stat_population)

    if not user_scores:
        return sample

    simulated_worst = {
        "value": user_scores[-1]["value"],
        "estimated_rank": stat_population
    }
    ranked_users = []
    i = 0
    while i < sample_size and user_scores:
        next_rank_index = get_next_rank_index(sample, i)
        upper_bound = sample[i]
        lower_bound = simulated_worst
        if next_rank_index != sample_size:
            lower_bound = sample[next_rank_index]
        ranked, user_scores = rank_users(upper_bound, lower_bound, user_scores)
        ranked_users += ranked
        i = next_rank_index

    return sort_leaderboard(sample + ranked_users, stat)


def get_next_rank_index(sample, i):
    rank = sample[i]["estimated_rank"]
    while i < len(sample):
        if sample[i]["estimated_rank"] != rank:
            return i
        i = i+1
    return len(sample)


def rank_users(upper, lower, user_scores):
    if not user_scores:
        return [], []

    if user_scores[0]["value"] < lower["value"]:
        return [], user_scores

    index = 1
    while index < len(user_scores) and user_scores[index]["value"] < lower["value"]:
        index = index + 1

    scores_to_rank = user_scores[:index]
    unranked = user_scores[index:]

    upper_value = float(upper["value"])
    lower_value = float(lower["value"])
    rank_range = lower["estimated_rank"] - upper["estimated_rank"]
    score_range = upper_value - lower_value
    for i in range(0, len(scores_to_rank)):
        score_position = (upper_value - float(scores_to_rank[i]["value"])) / score_range
        scores_to_rank[i]["estimated_rank"] = int(score_position * rank_range) + upper["estimated_rank"]

    return scores_to_rank, unranked



def estimate_sample_ranks(sample, population):
    if not sample:
        return []
    sample_size = len(sample)
    sample[0]["estimated_rank"] = 1
    for i in range(1, sample_size):
        if sample[i]["value"] == sample[i-1]["value"]:
            sample[i]["estimated_rank"] = sample[i-1]["estimated_rank"]
        else:
            sample[i]["estimated_rank"] = int(float(i) / float(sample_size) * int(population)) + 1
    return sample