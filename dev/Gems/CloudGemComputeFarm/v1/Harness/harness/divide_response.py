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

import uuid


class Task(object):
    def __init__(self, task_input, path_leaf):
        self.task_id = str(uuid.uuid4())
        self.task_input = task_input
        self.path_leaf = path_leaf


class DivideResponse(object):
    def __init__(self, path):
        self.merge_task = Task([], path)
        self.divide_tasks = []

    def as_dict(self):
        return {
            'merge_task_id': self.merge_task.task_id,
            'merge_input': self.merge_task.task_input,
            'merge_path': self.merge_task.path_leaf,
            'divide_task_ids': [task.task_id for task in self.divide_tasks],
            'divide_inputs': [task.task_input for task in self.divide_tasks],
            'divide_paths': [task.path_leaf for task in self.divide_tasks]
        }

    def set_leaf_task(self, task_input):
        self.merge_task.task_input = task_input

    def add_divide_task(self, task_input, path_leaf):
        self.divide_tasks.append(Task(task_input, path_leaf))
