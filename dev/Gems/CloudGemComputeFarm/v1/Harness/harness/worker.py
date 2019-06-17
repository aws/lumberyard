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

import datetime
import json
import os
import threading
import time
import traceback
import urllib

import botocore
import botocore.exceptions
import botocore.errorfactory


class Heartbeat(threading.Thread):
    def __init__(self, owner):
        threading.Thread.__init__(self)
        self.owner = owner
        self.config = owner.config
        self.swf_client = self.config.session.client("swf", config=self.config.swf_config, region_name=self.config.region)
        self.task_token = None
        self.lock = threading.Lock()
        self.cancelRequested = False

    def run(self):
        while True:
            self.lock.acquire()
            task_token = self.task_token
            self.lock.release()

            if task_token:
                try:
                    heartbeat = self.config.swf_client.record_activity_task_heartbeat(
                        taskToken=task_token
                    )
                    self.cancelRequested = heartbeat.get('cancelRequested', False)
                except botocore.exceptions.ClientError as e:
                    if e.response['Error']['Code'] != 'UnknownResourceFault':
                        print('Error on record_activity_task_heartbeat Error code {} : response {} : {}'.format(e.response['Error']['Code'], e.response, e))
                except Exception as e:
                    # This can fail on occasion if the task is already marked as complete
                    print('Error on record_activity_task_heartbeat : {}'.format(e))
                    pass

            time.sleep(60)

    def set_task_token(self, task_token):
        self.lock.acquire()
        self.task_token = task_token
        self.lock.release()


def _format_dynamo_value(item):
    if item is None:
        return {'NULL': True}
    elif isinstance(item, int):
        return {'N': str(item)}
    else:
        return {'S': str(item)}


class WorkflowCancelled(Exception):
    pass


class WorkerUpdater(object):
    def __init__(self, config):
        self.config = config
        self.workflow_execution = None
        self.workflow_id = None
        self.heartbeat = Heartbeat(self)
        self.task_description = None
        self.activity_id = None
        self.phase_index = None
        self.path = None
        self.log_token = None
        self.sequence_id = 0

        if self.config.log_group:
            try:
                self.config.log_client.delete_log_stream(
                    logGroupName=self.config.log_group,
                    logStreamName=self.config.identity
                )
            except:
                pass

            self.config.log_client.create_log_stream(
                logGroupName=self.config.log_group,
                logStreamName=self.config.identity
            )

        self.heartbeat.start()

    def set_task(self, task, path, phase_index):
        self.heartbeat.set_task_token(task['taskToken'])
        self.activity_id = task['activityId']
        self.phase_index = phase_index
        self.workflow_execution = task['workflowExecution']
        self.workflow_id = self.workflow_execution['workflowId']
        self.path = json.dumps(path)

    def clear_task(self):
        self.heartbeat.set_task_token(None)
        self.task_description = None
        self.activity_id = None
        self.phase_index = None
        self.path = None

    def begin_task(self, description):
        self.task_description = description
        self.post_task_update(None)

    def end_task(self):
        self.heartbeat.set_task_token(None)
        self.task_description = None
        self.activity_id = None
        self.phase_index = None
        self.post_task_update(None)

        self.path = None
        self.workflow_execution = None
        self.workflow_id = None

    def post_task_update(self, status):
        if self.heartbeat.cancelRequested:
            raise WorkflowCancelled()

        timestamp = datetime.datetime.utcnow().isoformat(timespec="seconds")
        update_info = {
            'run-id': json.dumps(self.workflow_execution) if self.workflow_execution else "-",
            'event-id': "%s.%s.%05d" % (timestamp, self.config.identity, self.sequence_id),
            'host-id': self.config.identity,
            'timestamp': timestamp,
            'activity': self.activity_id,
            'phase': self.phase_index,
            'desc': self.task_description,
            'path': self.path,
            'status': status
        }
        self.sequence_id += 1
        update_json = json.dumps(update_info)
        print(update_json)

        if self.config.log_db:
            params = {
                'TableName': self.config.log_db,
                'Item': {k: _format_dynamo_value(v) for k, v in update_info.items()}
            }
            self.config.dynamo_client.put_item(**params)


def run_worker(config):
    updater = WorkerUpdater(config)
    updater.post_task_update(None)

    while True:
        task = config.swf_client.poll_for_activity_task(
            domain=config.domain,
            taskList={
                'name': config.task_list_name
            }
        )

        if 'taskToken' not in task:
            continue

        try:
            data = json.loads(task['input'])
            activity_type = task['activityType']['name']
            updater.set_task(task, data['path'], config.task_index_lookup[activity_type])
            run_task = config.task_lookup[activity_type]

            result = run_task.handler(config, task, updater, data['main'], data['path'], data['merge'])
            config.swf_client.respond_activity_task_completed(
                taskToken=task['taskToken'],
                result=json.dumps(result)
            )

        except WorkflowCancelled:
            config.swf_client.respond_activity_task_canceled(
                taskToken=task['taskToken']
            )

        except Exception as e:
            message = "Error - " + str(e) + "\n" + traceback.format_exc()
            print(message)

            try:
                config.swf_client.respond_activity_task_failed(
                    taskToken=task['taskToken'],
                    reason="An error occurred while processing this task.",
                    details=str(message)
                )
            except:
                pass

        updater.end_task()
