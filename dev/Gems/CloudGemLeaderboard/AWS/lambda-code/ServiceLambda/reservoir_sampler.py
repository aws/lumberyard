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
import CloudCanvas
import datetime
import leaderboard_utils
import boto3
import random
import json
from boto3.dynamodb.conditions import Key


class ReservoirSampler:
    def __init__(self, stat):
        self.sample = []
        self.stat = stat

    '''
    if the sample is out of sync enough to warrant a reload
    '''
    def is_expired(self):
        return False

    '''
    get the current sample
    '''
    def get_sample(self):
        if self.is_expired():
            print("reloading sample set")
            self.reload_sample()
        return self.sample

    '''
    recreate the sample (load from AWS, or regenerate on the fly)
    '''
    def reload_sample(self):
        pass

    def get_full_dataset_size(self):
        return 0



class OnTheFlyReservoirSampler(ReservoirSampler):
    def __init__(self, stat, size = 200):
        ReservoirSampler.__init__(self, stat)
        self.expiration_time =  datetime.datetime.now() + datetime.timedelta(minutes = 5)
        self.full_dataset_size = 0
        self.size = size
        try:
            self.record_table = boto3.resource('dynamodb').Table(CloudCanvas.get_setting("MainTable"))
        except Exception as e:
            raise Exception("Error getting table reference")
        self.reload_sample()

    def is_expired(self):
        return datetime.datetime.now() > self.expiration_time

    def reload_sample(self):
        self.expiration_time =  datetime.datetime.now() + datetime.timedelta(minutes = 5)
        self.sample = []
        try:
            response = self.record_table.scan()
            for item in response.get("Items", []):
                score = leaderboard_utils.generate_score_dict(item, self.stat)
                if score:
                    self.add_to_sample(score)

            while "LastEvaluatedKey" in response:
                response = self.record_table.scan(ExclusiveStartKey=response['LastEvaluatedKey'])
                for item in response.get("Items", []):
                    score = leaderboard_utils.generate_score_dict(item, self.stat)
                    if score:
                        self.add_to_sample(score)

        except Exception as e:
            raise Exception("Error fetching scores from table")

        self.sample = leaderboard_utils.sort_leaderboard(self.sample, self.stat)

    def add_to_sample(self, score):
        self.full_dataset_size = self.full_dataset_size + 1
        if len(self.sample) < self.size:
            self.sample.append(score)
        elif random.random() < (1.0 / float(len(self.sample))):
            self.sample.pop(random.randint(0, len(self.sample) - 1))
            self.sample.append(score)

    def get_full_dataset_size(self):
        return self.full_dataset_size


class DynamoDBBackedReservoirSampler(ReservoirSampler):
    def __init__(self, stat):
        ReservoirSampler.__init__(self, stat)
        self.leaderboard_table = boto3.resource('dynamodb').Table(CloudCanvas.get_setting("LeaderboardTable"))
        self.info_table = boto3.resource('dynamodb').Table(CloudCanvas.get_setting("LeaderboardInfo"))
        self.reload_sample()


    def reload_sample(self):
        try:
            new_sample = []
            response = self.leaderboard_table.query(
                KeyConditionExpression=Key('type').eq('{}_sample'.format(self.stat))
            )
            for entry in response.get("Items", []):
                new_sample.append(self.generate_score_dict(entry))

            while "LastEvaluatedKey" in response:
                response = self.leaderboard_table.query(
                    KeyConditionExpression=Key('type').eq('{}_sample'.format(self.stat))
                )
                for entry in response.get("Items", []):
                    new_sample.append(self.generate_score_dict(entry))
            high_score = json.loads(self.get_leaderboard_info("high_score"))
            if high_score:
                high_score = self.generate_score_dict(high_score)
                if not high_score["user"] in [entry["user"] for entry in new_sample]:
                    new_sample.append(high_score)
        except Exception as e:
            raise Exception("Error loading {} leaderboard".format(self.stat))

        self.sample = leaderboard_utils.sort_leaderboard(new_sample, self.stat)
        self.expiration_time =  datetime.datetime.now() + datetime.timedelta(seconds = 30)


    def generate_score_dict(self, entry):
        return {
            "user": entry["user"],
            "stat": self.stat,
            "value": entry["value"]
        }

    def is_expired(self):
        return datetime.datetime.now() > self.expiration_time

    def get_full_dataset_size(self):
        return self.get_leaderboard_info("population")

    def get_leaderboard_info(self, info):
        response = self.info_table.get_item(Key = {"stat": self.stat, "type": info })
        if not response.get("Item", {}):
            raise Exception("Error retrieving leaderboard info for {}".format(self.stat))

        return response["Item"]["value"]
