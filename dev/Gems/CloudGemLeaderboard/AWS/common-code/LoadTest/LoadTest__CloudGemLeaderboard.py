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
# $Revision$

from __future__ import print_function
from cloud_gem_load_test.service_api_call import ServiceApiCall
from random import choice
from requests_aws4auth import AWS4Auth
from time import time

import leaderboard_constant as c
import requests


#
# Load Test Transaction Handler registration
#
def add_transaction_handlers(handler_context, transaction_handlers):
    service_api_name = c.GEM_NAME + '.ServiceApi'
    base_url = handler_context.mappings.get(service_api_name, {}).get('PhysicalResourceId')
    if not base_url:
        raise RuntimeError('Missing PhysicalResourceId for ' + service_api_name)

    transaction_handlers.append(ServiceStatus(base_url=base_url))
    transaction_handlers.append(NewHighScore(base_url=base_url))
    transaction_handlers.append(DiscardedScore(base_url=base_url))
    transaction_handlers.append(GetAllUserScores(base_url=base_url))
    transaction_handlers.append(GetSingleUserScore(base_url=base_url))
    transaction_handlers.append(GetLeaderboard(base_url=base_url))


#
# API Handler Initialization
#
class LeaderboardCall(ServiceApiCall):
    def __init__(self, **kwargs):
        kwargs['name'] = c.GEM_NAME + '.' + kwargs['name']
        super(LeaderboardCall, self).__init__(**kwargs)

    def initialize(self, initialize_context):
        super(LeaderboardCall, self).initialize(initialize_context)
        self.stats = initialize_context.load_test_parameters.get(c.GEM_NAME, {}).get('stats', [])


#
# Check for the service status of Cloud Gem Under Test
#
class ServiceStatus(LeaderboardCall):
    def __init__(self, **kwargs):
        super(ServiceStatus, self).__init__(name='ServiceStatus', method='get', path='/service/status', **kwargs)


#
# Add new high score to score table API transaction
#
class NewHighScore(LeaderboardCall):
    def __init__(self, name='NewHighScore', **kwargs):
        super(NewHighScore, self).__init__(name=name, method='post', path='/score', **kwargs)

    def build_request(self):
        request = super(NewHighScore, self).build_request()
        request['body'] = {
            'stat': choice(self.stats),
            'user': request['auth']['identity'],
            'value': get_new_high_score()
        }
        return request


#
# Add new discarded high score transaction
#
class DiscardedScore(NewHighScore):
    def __init__(self, **kwargs):
        super(DiscardedScore, self).__init__(name='DiscardedScore', **kwargs)

    def build_request(self):
        request = super(DiscardedScore, self).build_request()
        request['body']['value'] = get_discarded_score()
        return request


#
# Get All User Scores API transaction
#
class GetAllUserScores(LeaderboardCall):
    def __init__(self, **kwargs):
        super(GetAllUserScores, self).__init__(name='GetAllUserScores', method='get', path='/scores/{user}', **kwargs)

    def build_request(self):
        request = super(GetAllUserScores, self).build_request()
        request['url'] = request['url'].format(user=request['auth']['identity'])
        return request


#
# Get score from individual user API transaction
#
class GetSingleUserScore(LeaderboardCall):
    def __init__(self, **kwargs):
        super(GetSingleUserScore, self).__init__(name='GetSingleUserScore', method='get', path='/score/{stat}/{user}', **kwargs)

    def build_request(self):
        request = super(GetSingleUserScore, self).build_request()
        request['url'] = request['url'].format(stat=choice(self.stats), user=request['auth']['identity'])
        return request


#
# Get the leaderboard API transaction
#
class GetLeaderboard(LeaderboardCall):
    def __init__(self, **kwargs):
        super(GetLeaderboard, self).__init__(name='GetLeaderboard', method='post', path='/leaderboard/{stat}', body={}, **kwargs)
        self.__stats = ['score']

    def build_request(self):
        request = super(GetLeaderboard, self).build_request()
        request['url'] = request['url'].format(stat=choice(self.stats))
        return request


#
# Works if the rate is far enough under 1000 calls per second.
#
def get_new_high_score():
    return int(time() * 1000)

#
# First round of requests will probably be accepted, and the rest discarded as duplicates.
#
def get_discarded_score():
    return 1

#
# Get AWS Cognito credentials
#
def get_auth_signer(context):
    creds = context.session.get_credentials().get_frozen_credentials()
    return AWS4Auth(creds.access_key, creds.secret_key, context.region, 'execute-api', session_token=creds.token)
