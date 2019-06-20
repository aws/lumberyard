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

import botocore
import botocore.client


class TaskConfig(object):
    def __init__(self, name, version, handler):
        self.name = name
        self.version = version
        self.handler = handler
        self.as_dict = {'name': name, 'version': version}


class Config(object):
    def __init__(self, session, region, domain, task_list_name, div_task, build_task, merge_task, log_group, log_db,
                 kvs_db, config_bucket, identity):
        self.session = session
        self.region = region
        self.domain = domain
        self.task_list_name = task_list_name
        self.div_task = div_task
        self.build_task = build_task
        self.merge_task = merge_task
        self.swf_config = botocore.client.Config(connect_timeout=50, read_timeout=70)
        self.swf_client = self.session.client("swf", config=self.swf_config, region_name=region)
        self.s3_client = self.session.client("s3")
        self.s3_resource = self.session.resource("s3")
        self.log_client = self.session.client("logs", region_name=region)
        self.log_group = log_group
        self.log_db = log_db
        self.kvs_db = kvs_db
        self.dynamo_client = self.session.client("dynamodb", region_name=region)
        self.config_bucket = config_bucket
        self.identity = identity
        self.task_lookup = {task.name: task for task in (div_task, build_task, merge_task)}
        self.task_index_lookup = {task.name: idx for idx, task in enumerate((div_task, build_task, merge_task))}
