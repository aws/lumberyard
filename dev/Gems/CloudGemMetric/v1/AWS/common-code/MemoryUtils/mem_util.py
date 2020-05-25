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

import time
import metric_constant as c
import os
import psutil

def get_memory_usage():   
    memory = psutil.virtual_memory()        
    return memory.percent

def get_process_memory_usage_bytes():
    pid = os.getpid()
    py = psutil.Process(pid)
    memoryUseBytes = py.memory_info()[0]/2        
    return memoryUseBytes

def get_process_memory_usage_kilobytes():           
    return get_process_memory_usage_bytes()/1024

def get_process_memory_usage_megabytes():       
    return get_process_memory_usage_kilobytes()/1024

def get_memory_object():
    return psutil.virtual_memory()  