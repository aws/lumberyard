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
import boto3
import json
import random
from boto3.dynamodb.conditions import Key
from decimal import *


MAIN_TABLE = None
SHARD_START_POINTS = {}


def __init_resources():
    global MAIN_TABLE
    MAIN_TABLE = boto3.resource('dynamodb').Table(CloudCanvas.get_setting("MainTable"))

def load_shard_status(shard_sequence_table):
    try:
        response = shard_sequence_table.scan()
        for item in response.get("Items", []):
            SHARD_START_POINTS[item["shard_id"]] = item["last_sequence"]

        while "LastEvaluatedKey" in response:
            response = shard_sequence_table.scan(ExclusiveStartKey=response['LastEvaluatedKey'])
            for item in response.get("Items", []):
                SHARD_START_POINTS[item["shard_id"]] = item["last_sequence"]
    except Exception as e:
        raise Exception("Failure to read from ScoreStreamStatus table")


def get_oldest_ancestor(shard_id, shard_dict):
    parent_id = shard_dict[shard_id].get("ParentShardId", "")
    if shard_dict.get(parent_id, {}):
        if shard_dict[parent_id]["AlreadyProcessed"]:
            return shard_id
        return get_oldest_ancestor(parent_id, shard_dict)
    return shard_id


class ShardReader:
    def __init__(self, shard, stream_client, stream_arn, tries=20, read_size=1000):
        self.shard = shard
        self.stream_client = stream_client
        self.stream_arn = stream_arn
        self.tries = tries
        self.read_size = read_size
    
    def get_records(self, shard_start_points):
        start = shard_start_points.get(self.shard["ShardId"], self.shard["SequenceNumberRange"]["StartingSequenceNumber"])
        end = self.shard["SequenceNumberRange"].get("EndingSequenceNumber", "")

        if start == end:
            return []
        if end:
            print("reading from {} to {} for shard {}".format(start, end, self.shard["ShardId"]))
        else:
            print("reading open stream {} for shard {}".format(start, self.shard["ShardId"]))

        iterator_info = {
            "start_sequence_number": start,
            "iterator_type": 'AT_SEQUENCE_NUMBER',
            "end_sequence_number": end
        }

        # if we had an override from stream status table
        if start != self.shard["SequenceNumberRange"]["StartingSequenceNumber"]:
            iterator_info["iterator_type"] = "AFTER_SEQUENCE_NUMBER"

        
        if iterator_info["end_sequence_number"]:
            return self.read_shard_to_end(iterator_info)
        return self.read_open_shard(iterator_info)


    def read_shard_to_end(self, iter_info):
        shard_id = self.shard["ShardId"]
        response = self.stream_client.get_shard_iterator(StreamArn=self.stream_arn,
                                                    ShardId=shard_id,
                                                    ShardIteratorType=iter_info["iterator_type"],
                                                    SequenceNumber=iter_info["start_sequence_number"])
        iterator = response['ShardIterator']

        response = self.stream_client.get_records(ShardIterator=iterator, Limit=self.read_size)
        records = response.get("Records", [])

        end_sequence = iter_info["end_sequence_number"]

        # looking for first entry
        while not records:
            iterator = response["NextShardIterator"]
            response = self.stream_client.get_records(ShardIterator=iterator, Limit=self.read_size)
            records = response.get("Records", [])

        # Now read to the end
        while records[-1]["dynamodb"]["SequenceNumber"] != end_sequence:
            iterator = response["NextShardIterator"]
            response = self.stream_client.get_records(ShardIterator=iterator, Limit=self.read_size)
            records = records + response.get("Records", [])

        return records
    

    def read_open_shard(self, iter_info):
        shard_id = self.shard["ShardId"]

        response = self.stream_client.get_shard_iterator(StreamArn=self.stream_arn,
                                                        ShardId=shard_id,
                                                        ShardIteratorType=iter_info["iterator_type"],
                                                        SequenceNumber=iter_info["start_sequence_number"])
        iterator = response['ShardIterator']

        response = self.stream_client.get_records(ShardIterator=iterator, Limit=self.read_size)
        records = response.get("Records", [])

        tries = self.tries
        while tries > 0:
            tries = tries - 1
            iterator = response["NextShardIterator"]
            response = self.stream_client.get_records(ShardIterator=iterator, Limit=self.read_size)
            records = records + response.get("Records", [])

        return records



class RecordProcessor:
    def __init__(self, leaderboard_writer, shard_sequence_table):
        self.leaderboard_writer = leaderboard_writer
        self.shard_sequence_table = shard_sequence_table

    def process_records(self, records, shard_id):
        if not records:
            return
        last_sequence = records[-1]["dynamodb"]["SequenceNumber"]
        records = self.crunch_records(records)

        self.leaderboard_writer.write_to_leaderboard(records)
        self.record_last_sequence(shard_id, last_sequence)


    '''
    processes a set of records, reducing them to the smallest set of writes
    '''
    def crunch_records(self, records):
        # the records will come in order, so we should be able to consolidate records to unique PLAYERxSTAT entry without dropping scores
        record_map = {}
        for record in records:
            abbreviated_record = self.get_abbreviated_record(record)
            if abbreviated_record["user"] in record_map:
                record = self.resolve_record(abbreviated_record, record_map[abbreviated_record["user"]].get(abbreviated_record["stat"], {}))
                record_map[abbreviated_record["user"]][abbreviated_record["stat"]] = record
            else:
                record_map[abbreviated_record["user"]] = { abbreviated_record["stat"]: abbreviated_record }

        consolidated_records = []
        for user, user_scores in record_map.iteritems():
            for score, record in user_scores.iteritems():
                if record:
                    consolidated_records.append(record)

        return consolidated_records


    def resolve_record(self, incoming, existing):
        # happens on a deleted user, this isnt as clean of an operation, and we do not support it through the APIs, so there is certainly some loss of fidelity here (we do not go through and delete each score)
        if incoming.get("status", "") == "DED":
            return {}

        if incoming.get("record_type") == "DELETE":
            return {
                "user": incoming["user"],
                "stat": incoming["stat"],
                "status" : "DELETE"
                }

        # keep deleted status if it has been deleted in this stream and re-added
        if existing.get("status", "") == "DELETE":
            incoming["status"] = "DELETE"
        # if this score is new in this stream, keep the "NEW" status
        if existing.get("record_type", "") == "NEW":
            incoming["record_type"] = "NEW"
        return incoming

    def get_abbreviated_record(self, record):
        if record['eventName'] == "REMOVE":
            print("A whole player has been deleted, you may need to regenerate your leaderboard samples")
            return { "user": record["dynamodb"]['Keys']['user']['S'], "status": "DED"}

        record_type, stat = self.get_changed_stat(record)

        abbreviated =  {
            "user": record["dynamodb"]['Keys']['user']['S'],
            "stat": stat,
            "value": self.get_new_value(record, stat)
            }
        if record_type == "DELETE":
            abbreviated["status"] = "DELETE"
        else:
            abbreviated["record_type"] = record_type
        
        return abbreviated

    def get_changed_stat(self, record):
        entry = record["dynamodb"]
        record_type = None
        stat = None
        new_fields = entry.get('NewImage', {}).keys()
        old_fields = entry.get('OldImage', {}).keys()

        added_fields = [field for field in new_fields if field not in old_fields]
        if "user" in added_fields:
            added_fields.remove("user")

        if len(added_fields) > 1: # too much stuff happened here, not sure how to resolve this record
            print("There was more than one edit this operation, touched {}".format(added_fields))
        elif added_fields: # this was the players first submission for this score
            record_type = "NEW"
            stat = added_fields[0]
        else: # this is an updated score
            update_fields = []
            for field in new_fields:
                if entry['NewImage'][field] != entry['OldImage'][field]:
                    update_fields.append(field)
            if len(update_fields) > 1: # too much stuff happened here, not sure how to resolve this record
                print("There was more than one edit this operation, touched {}".format(update_fields))
            elif update_fields:
                record_type = "UPDATE"
                stat = update_fields[0]

        if stat == None: # this must be a deletion
            removed_fields = [field for field in old_fields if field not in new_fields]
            if len(removed_fields) > 1: # too much stuff happened here, not sure how to resolve this record
                print("There was more than one edit this operation, touched {}".format(removed_fields))
            elif removed_fields: # this was the players first submission for this score
                record_type = "DELETE"
                stat = removed_fields[0]
        return record_type, stat


    def get_new_value(self, record, stat):
        entry = record["dynamodb"]
        val_string = entry.get('NewImage', {}).get(stat, {}).get('N', '')
        if not val_string:
            return None
        return Decimal(Decimal(val_string).quantize(Decimal('.000000'), rounding=ROUND_HALF_UP))


    '''
    record last read position in stream so you know where to start from next time
    '''
    def record_last_sequence(self, shard_id, last_sequence):
        print("last sequence = {}".format(last_sequence))
        self.shard_sequence_table.put_item(Item={ "shard_id": shard_id, "last_sequence": last_sequence})



class LeaderboardWriter:
    def __init__(self, leaderboard_table_name, info_table_name):
        # getting refs is expensive, so don't do it until we need to
        self.leaderboard_table = None
        self.info_table = None

        self.leaderboard_table_name = leaderboard_table_name
        self.info_table_name = info_table_name

        self.leaderboard_info = {}
        self.sample_sets = {}
    

    def get_leaderboard_table(self):
        if not self.leaderboard_table:
            self.leaderboard_table = boto3.resource('dynamodb').Table(self.leaderboard_table_name)
        return self.leaderboard_table

    def get_info_table(self):
        if not self.info_table:
            self.info_table = boto3.resource('dynamodb').Table(self.info_table_name)
        return self.info_table

    def get_leaderboard_info(self, stat_name):
        info_table = self.get_info_table()
        if not stat_name in self.leaderboard_info:
            self.leaderboard_info[stat_name] = {}
            response = info_table.query(
                KeyConditionExpression=Key("stat").eq(stat_name)
            )
            for entry in response.get("Items", []):
                self.leaderboard_info[stat_name][entry['type']] = entry['value']
        return self.leaderboard_info[stat_name]

    def get_all_entries(self, stat_name, force_reload=False):
        leaderboard = self.get_leaderboard_table()
        if force_reload or not stat_name in self.sample_sets:
            self.sample_sets[stat_name] = []
            response = leaderboard.query(
                KeyConditionExpression=Key('type').eq('{}_sample'.format(stat_name))
            )
            for entry in response["Items"]:
                self.sample_sets[stat_name].append(entry)

            while "LastEvaluatedKey" in response:
                response = leaderboard.query(
                    KeyConditionExpression=Key('type').eq('{}_sample'.format(stat_name)),
                    ExclusiveStartKey = response['LastEvaluatedKey']
                )
                for entry in response["Items"]:
                    self.sample_sets[stat_name].append(entry)

        return self.sample_sets[stat_name]


    def num_entries(self, stat_name):
        return len(self.get_all_entries(stat_name))

    
    def find_entry(self, stat_name, user):
        entries = self.get_all_entries(stat_name)
        for entry in entries:
            if entry["user"] == user:
                return entry
        return None

    
    def write_to_leaderboard(self, records):
        population_changes = {}
        sample_size_changes = {}
        high_scores = {}
        with self.get_leaderboard_table().batch_writer() as batch:
            for record in records:
                # if record type is update, look for existing entry
                if record.get("record_type", "") == "UPDATE":
                    high_scores = self.update_high_scores(high_scores, record)
                    self.add_or_update_entry(record, batch)
                elif record.get("record_type", "") == "NEW":
                    high_scores = self.update_high_scores(high_scores, record)
                    if self.should_add_to_sample(record, population_changes.get(record["stat"], 0)):
                        added = self.add_or_update_entry(record, batch, new_entry=True)
                        if record.get("status") != "DELETE":
                            # This was not a delete + add, so total population went up.
                            population_changes[record["stat"]] = population_changes.get(record["stat"], 0) + 1
                            if added:
                                sample_size_changes[record["stat"]] = sample_size_changes.get(record["stat"], 0) + 1
                    elif record.get("status") == "DELETE":
                        # this was a delete + add, but failed check to get into sample, so we should delete entry if it is there
                        if self.delete_entry_if_exists(record, batch):
                            sample_size_changes[record["stat"]] = sample_size_changes.get(record["stat"], 0) - 1

                elif record.get("status") == "DELETE":
                    if self.delete_entry_if_exists(record, batch):
                        sample_size_changes[record["stat"]] = sample_size_changes.get(record["stat"], 0) - 1
                    population_changes[record["stat"]] = population_changes.get(record["stat"], 0) - 1
                    
                else:
                    print("the following record slipped through during write_to_leaderboard: {}".format(record))

        self.record_population_changes(population_changes)
        self.record_top_scores(high_scores)
            
        # handle evictions
        with self.get_leaderboard_table().batch_writer() as batch:
            for stat, sample_size_change in sample_size_changes.iteritems():
                if sample_size_change == 0:
                    continue
                sample = self.get_all_entries(stat, True)
                diff = len(sample) - self.get_leaderboard_info(stat)["max_sample_size"] 
                self.evict_sample_entries(sample, diff, batch)


    def evict_sample_entries(self, sample, diff, batch):
        while diff > 0:
            eviction = random.choice(sample)
            print("evicting entry: {}".format(eviction))
            sample.remove(eviction)
            key = {
                "type": eviction["type"],
                "user": eviction["user"]
            }
            batch.delete_item(Key=key)
            diff = diff - 1


    def update_high_scores(self, high_scores, record):
        if not record["stat"] in high_scores:
            high_scores[record["stat"]] = record
        elif record["value"] > high_scores[record["stat"]]["value"]:
            high_scores[record["stat"]] = record
        return high_scores


    def should_add_to_sample(self, record, population_change):
        stat = record["stat"]
        max_sample_size = self.get_leaderboard_info(stat)["max_sample_size"]
        if self.num_entries(stat) + population_change < max_sample_size:
            return True
        return random.randint(0, self.get_leaderboard_info(stat)["population"] + population_change) <= max_sample_size


    def record_top_scores(self, high_scores):
        for stat, score in high_scores.iteritems():
            current_high_score = json.loads(self.get_leaderboard_info(stat)["high_score"])
            if not "value" in current_high_score:
                self.get_info_table().put_item(Item={
                    "stat": stat,
                    "type": "high_score",
                    "value": json.dumps({"user": score["user"], "value": float(score["value"])})
                })
            elif score["value"] > current_high_score["value"]:
                self.get_info_table().put_item(Item={
                    "stat": stat,
                    "type": "high_score",
                    "value": json.dumps({"user": score["user"], "value": float(score["value"])})
                })


    def record_population_changes(self, changes):
        if not changes:
            return
        info_table = self.get_info_table()
        with info_table.batch_writer() as batch:
            for stat, change in changes.iteritems():
                print("population change for {} =  {}".format(stat, change))
                item = {
                    "stat": stat,
                    "type": "population",
                    "value": self.get_leaderboard_info(stat)["population"] + change
                }
                batch.put_item(Item=item)


    '''
    Looks if entry is in sample and removes it from the table if it is.
    Returns True if delete took place
    '''
    def delete_entry_if_exists(self, record, batch):
        if not self.find_entry(record["stat"], record["user"]):
            print("Do not have to delete entry for {} {}, because it is not in sample".format(record.get("user"), record.get("stat")))
            return False
        print("deleting entry for {} {}".format(record.get("user"), record.get("stat")))
        key = {
            "type": "{}_sample".format(record["stat"]),
            "user": record["user"]
        }
        batch.delete_item(Key=key)
        return True

    '''
    Looks if entry already exists in sample and updates
    if new_entry is True will add to sample
    Returns True if a new entry was added (not an update)
    '''
    def add_or_update_entry(self, record, batch, new_entry=False):
        entry_exists = self.find_entry(record["stat"], record["user"])
        if not new_entry and not entry_exists:
            print("skipping put for {} {} because it is an update for something that is not in the sample".format(record.get("user"), record.get("stat")))
            return
        print("put item for {} {}".format(record.get("user"), record.get("stat")))
        item = {
            "type": "{}_sample".format(record["stat"]),
            "user": record["user"],
            "value": record["value"]
        }
        batch.put_item(Item=item)

        return new_entry and (entry_exists == None)


def main(event, context):
    global MAIN_TABLE
    __init_resources()
    stream_client = boto3.client('dynamodbstreams')

    shard_sequence_table = boto3.resource('dynamodb').Table(CloudCanvas.get_setting("ScoreStreamStatus"))

    load_shard_status(shard_sequence_table)

    stream_arn = MAIN_TABLE.latest_stream_arn
    response = stream_client.describe_stream(StreamArn=stream_arn)
    # Get shards for stream
    shards = response.get("StreamDescription", {}).get("Shards", [])
    print("stream has {} shards".format(len(shards)))
    shard_dict = {}
    for shard in shards:
        shard_id = shard["ShardId"]
        shard_dict[shard_id] = shard
        shard_dict[shard_id]["AlreadyProcessed"] = False

    writer = LeaderboardWriter(CloudCanvas.get_setting("LeaderboardTable"),
                               CloudCanvas.get_setting("LeaderboardInfo"))
    record_processor = RecordProcessor(writer, shard_sequence_table)

    for shard_id, shard in shard_dict.iteritems():
        # read the oldest ancestor so we process the records in order
        id_to_read = get_oldest_ancestor(shard_id, shard_dict)
        while (id_to_read != shard_id):
            shard_reader = ShardReader(shard_dict[id_to_read], stream_client, stream_arn)
            record_processor.process_records(shard_reader.get_records(SHARD_START_POINTS), shard_dict[id_to_read]["ShardId"])
            shard_dict[id_to_read]["AlreadyProcessed"] = True
            id_to_read = get_oldest_ancestor(shard_id, shard_dict)

        if not shard["AlreadyProcessed"]:
            shard_reader = ShardReader(shard, stream_client, stream_arn)
            record_processor.process_records(shard_reader.get_records(SHARD_START_POINTS), shard["ShardId"])
            shard_dict[shard_id]["AlreadyProcessed"] = True
