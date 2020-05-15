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

from six.moves.queue import Queue
from threading import Thread
from worker import Worker

"""
Inspired by Chris Hager's work
https://www.metachris.com/2016/04/python-threadpool/
"""
class ThreadPool:    
    def __init__(self, context={}, size=2):
        self.tasks = Queue(size)
        for num in range(size):
            Worker(self.tasks, context)

    def add(self, func, *args, **kargs):
        self.tasks.put((func, args, kargs))

    def wait(self):        
        self.tasks.join()
    
