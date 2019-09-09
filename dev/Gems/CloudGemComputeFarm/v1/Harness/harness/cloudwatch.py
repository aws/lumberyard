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
import sys
import time

from botocore.exceptions import ClientError

class OutputHandler(object):
    def __init__(self, log_group, session, regionname, identity, taskname):
        self.response = None
        self.write = self.first_write
        self.logClient = session.client('logs', region_name=regionname)
        self.log_group = log_group
        self.base_out = sys.stdout
        cur_time_str = time.strftime("%Y-%m-%d.%H-%M-%S")
        self.log_stream = '{}.{}.{}'.format(identity, cur_time_str, taskname)
        self.log_stream_error = 'Error.{}'.format(self.log_stream)
        self.error_response = None

        existing_group = self.logClient.describe_log_groups(logGroupNamePrefix=self.log_group)
        if not len(existing_group.get('logGroups', [])):
            self.logClient.create_log_group(logGroupName=self.log_group)
        existing_stream = self.logClient.describe_log_streams(logGroupName=self.log_group,
                                                              logStreamNamePrefix=self.log_stream)
        if not len(existing_stream.get('logStreams', [])):
            self.logClient.create_log_stream(logGroupName=self.log_group, logStreamName=self.log_stream)

        sys.stdout = self
        sys.stderr = self
        print('Initializing Harness')

    def write_error(self, message):
        cur_time = int(round(time.time() * 1000))
        try:
            if self.error_response:
                self.error_response = self.logClient.put_log_events(logGroupName=self.log_group,
                                                                    logStreamName=self.log_stream_error,
                                                                    logEvents=[{'timestamp': cur_time,
                                                                                'message': message}],
                                                                    sequenceToken=self.error_response[
                                                                        "nextSequenceToken"])
            else:
                existing_stream = self.logClient.describe_log_streams(logGroupName=self.log_group,
                                                                      logStreamNamePrefix=self.log_stream_error)
                if not len(existing_stream.get('logStreams', [])):
                    self.logClient.create_log_stream(logGroupName=self.log_group, logStreamName=self.log_stream_error)
                self.error_response = self.logClient.put_log_events(logGroupName=self.log_group,
                                                                    logStreamName=self.log_stream_error,
                                                                    logEvents=[{'timestamp': cur_time,
                                                                                'message': message}])
        except ClientError as e:
            self.base_write('Failed to put log event {} : {}'.format(message, e.response['Error']['Code']))
        except Exception as e:
            self.base_write('Failed to put log event {} : {}'.format(message, e))
            return


    def base_write(self, message):
        sys.stdout = self.base_out
        print(message)
        sys.stdout = self

    def first_write(self, someOutput):
        if 'Error' in someOutput:
            self.write_error(someOutput)
            return

        cur_time = int(round(time.time() * 1000))
        try:
            self.response = self.logClient.put_log_events(logGroupName=self.log_group, logStreamName=self.log_stream,
                                                          logEvents=[{'timestamp': cur_time,
                                                                      'message': someOutput}])
        except ClientError as e:
            self.base_write('Failed to put log event {} : {}'.format(someOutput, e.response['Error']['Code']))
        except Exception as e:
            self.base_write('Failed to put log event {} : {}'.format(someOutput, e))
            return

        self.write = self.token_write

    def token_write(self, someOutput):
        if 'Error' in someOutput:
            self.write_error(someOutput)
            return

        cur_time = int(round(time.time() * 1000))
        try:
            self.response = self.logClient.put_log_events(logGroupName=self.log_group, logStreamName=self.log_stream,
                                                          logEvents=[{'timestamp': cur_time,
                                                                      'message': someOutput}],
                                                          sequenceToken=self.response["nextSequenceToken"])
        except ClientError as e:
            self.base_write('Failed to put log event {} : {}'.format(someOutput, e.response['Error']['Code']))
        except Exception as e:
            self.base_write('Failed to put log event {} : {}'.format(someOutput, e))