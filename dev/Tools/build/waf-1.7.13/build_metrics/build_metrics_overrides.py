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

import build_metrics_reporter
import time
from waflib import Scripting, Task


'''
This file does the job of intercepting calls within WAF processes by 'monkey patching' (the
process of replacing a func with another func, which in turn calls the original func). In this
way we are able to have a minimal impact on build code as we do not have to instrument our
build with lots of calls into the metrics reporter. We are also easily able to add & remove
sections of reporting allowing us to dig into specific areas and report on them and then, once
an issue is resolved, simply remove the 'patch' - leaving the wrapper func available for
later use.
'''

# ========================== SCRIPTING ==========================
old_scripting_run_commands = Scripting.run_commands

def run_commands_gather_metrics():
    try:
        old_scripting_run_commands()
    except:
        raise
    finally:
        build_metrics_reporter.stop_metric_reporter(True, True)


old_scripting_run_command = Scripting.run_command

def run_command_gather_metrics(cmd_name):
    start_time = time.time()

    try:
        ret = old_scripting_run_command(cmd_name)
    except:
        raise
    finally:
        build_metrics_reporter.submit_build_metric('CommandTime',
                                                    build_metrics_reporter.MetricUnit['Seconds'],
                                                    time.time() - start_time,
                                                    {'Command': cmd_name },
                                                    True)
    return ret

Scripting.run_commands = run_commands_gather_metrics
Scripting.run_command = run_command_gather_metrics

# ========================== BUILD CONTEXT ==========================
old_taskbase_process = Task.TaskBase.process


def taskbase_process_monkey_patch(self):
    start_time = time.time()
    try:
        old_taskbase_process(self)
    except:
        raise
    finally:
        execution_time = time.time() - start_time
        build_metrics_reporter.submit_build_metric('TaskTime',
                                                   build_metrics_reporter.MetricUnit['Seconds'],
                                                   execution_time,
                                                   {'Task':type(self).__name__},
                                                   True, True)

Task.TaskBase.process = taskbase_process_monkey_patch

