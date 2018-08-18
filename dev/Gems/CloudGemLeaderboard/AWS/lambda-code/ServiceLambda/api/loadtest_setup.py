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

import service
import stats_settings
import leaderboard_constant as c


#
# Cloud Gem Leaderboard Load Test Setup Initialization API
#
@service.api
def post(request):
    print("{} load test initialization entered.".format(c.GEM_NAME))
    valid_stats = []

    stats_response = stats_settings.get_leaderboard_stats()
    existing_stats = set([stat['name'] for stat in stats_response])

    build_stats(existing_stats, valid_stats)

    print("{} load test initialization has finished.".format(c.GEM_NAME))
    return {'stats': valid_stats}


#
# Build & add stats and then append them to the output
#
def build_stats(existing_stats, valid_stats):
    for stat in range(c.NUM_STATS):
        name = 'score{}'.format(stat)
        if name not in existing_stats:
            stats_settings.add_stat({'name': name, 'mode': 'OVERWRITE'})
            print('Added new stat {} to {}'.format(name, c.GEM_NAME))
        valid_stats.append(name)
