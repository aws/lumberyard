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
# $Revision: #1 $

import os
import ctypes
import platform
import sys
import traceback
import time
import atexit

# Global reference to the lymetrics dll
lymetrics_dll = None

# Refresh interval for lymetrics publishing
process_interval_in_seconds = 2


def __get_lymetrics_library_path(path_override=None):
    if path_override:
        # for testing
        path = path_override
    else:
        # assumes this file is ...\dev\Gems\CloudGemFramework\v?\ResourceManager\resource_manager\metrics.py
        # we want ...\dev\Tools\InternalSDKs\LyMetrics\bin
        path = os.path.abspath(os.path.join(__file__, '..', '..', '..', '..', '..', '..'))
    path = os.path.join(path, 'Tools', 'InternalSDKs', 'LyMetrics', 'bin')
    if platform.system() == "Windows":
        path = os.path.join(path, 'windows', 'intel64')
        for build in ['Debug', 'Release']:
            dll = os.path.join(path, build, 'LyMetricsProducer_python.dll')
            if os.path.exists(dll):
                return dll
    return None


def __load_lymetrics_library(path_override=None):
    dll_path = __get_lymetrics_library_path(path_override)

    if dll_path is None:
        dll = None
    else:
        try:
            dll = ctypes.cdll.LoadLibrary(dll_path)
            dll.LyMetrics_CreateEvent.restype = ctypes.c_uint64
            dll.LyMetrics_Initialize("lmbraws", process_interval_in_seconds, True, None, None)
        except Exception as e:
            print("ERROR: Failed to load lymetrics library. {}".format(e))
            dll = None

    return dll


def get_lymetrics_library(path_override=None):
    global lymetrics_dll
    if lymetrics_dll is None:
        lymetrics_dll = __load_lymetrics_library(path_override=path_override)
    return lymetrics_dll


def _encode_string(value):
    """Encode a Python3 string in preparation to be passed across DLL boundary"""
    return ctypes.c_char_p(value.encode('utf-8'))


@atexit.register
def _unload_lymetrics_library():
    global lymetrics_dll
    if lymetrics_dll is not None:
        lymetrics_dll.LyMetrics_Shutdown()


class MetricsContext(object):
    """A wrapper for the LyMetricsProducer.dll library"""

    def __init__(self, origin):
        self.__dll = get_lymetrics_library()
        self.__startTime = time.time()
        self.__origin = origin
        self.__attributes = []
        self._command_name = None

    def __str__(self):
        return 'MetricsWrapper'

    # Internal
    #
    # LyMetrics forwarded interface
    def _create_event(self, name):
        if self.__dll is not None:
            return self.__dll.LyMetrics_CreateEvent(_encode_string(name))
        else:
            return -1

    def _submit_event(self, id):
        if self.__dll is not None:
            return self.__dll.LyMetrics_SubmitEvent(ctypes.c_uint64(id))
        else:
            return False

    def _cancel_event(self, id):
        if self.__dll is not None:
            return self.__dll.LyMetrics_CancelEvent(ctypes.c_uint64(id))
        else:
            return False

    def _add_attribute(self, id, key, value):
        if self.__dll is not None:
            return self.__dll.LyMetrics_AddAttribute(ctypes.c_uint64(id), _encode_string(key), _encode_string(value))
        else:
            return False

    def _add_metric(self, id, key, value):
        if self.__dll is not None:
            return self.__dll.LyMetrics_AddMetric(ctypes.c_uint64(id), _encode_string(key), value)
        else:
            return False

    def _set_context(self, key, value):
        if self.__dll is not None:
            self.__dll.LyMetrics_SetContext(_encode_string(key), _encode_string(value))

    def _clear_context(self, key):
        if self.__dll is not None:
            self.__dll.LyMetrics_ClearContext(_encode_string(key))

    # helper
    def _get_operation_name(self):
        return "".join([word.capitalize() for word in self._command_name.split('-')])

    def _submit_command_metric(self, status, add_duration, add_error):
        if self.__dll is None:
            return
        metric_id = self._create_event("ResourceManagement_Command")
        if metric_id == -1:
            print('ERROR: Cloud not create metrics event.')  # don't have access to context.view, this will do...
            return

        self._add_attribute(metric_id, "CommandOrigin", self.__origin)
        self._add_attribute(metric_id, "Operation", self._get_operation_name())
        self._add_attribute(metric_id, "Status", status)

        for attribute in self.__attributes:
            self._add_attribute(metric_id, attribute[0], attribute[1])

        if add_duration:
            time_elapsed = time.time() - self.__startTime
            self._add_metric(metric_id, "Duration", ctypes.c_double(time_elapsed))

        if add_error:
            _, e, tb = sys.exc_info()
            frames = traceback.extract_tb(tb)
            filename, line_no, _, _ = frames[len(frames) - 1]
            stripped_filename = os.path.basename(filename)

            self._add_attribute(metric_id, "ErrorType", e.__class__.__name__)
            self._add_attribute(metric_id, "FileName", stripped_filename)
            self._add_attribute(metric_id, "LineNumber", str(line_no))

        if not self._submit_event(metric_id):
            print('ERROR: Cloud not submit metrics event.')  # don't have access to context.view, this will do...
            return

    #  high-level lmbr_aws interface
    def submit_attempt(self):
        self._submit_command_metric("Attempt", add_duration=False, add_error=False)

    def submit_success(self):
        self._submit_command_metric("Success", add_duration=True, add_error=False)

    def submit_interrupted(self):
        self._submit_command_metric("Interrupted", add_duration=True, add_error=False)

    def submit_failure(self):
        self._submit_command_metric("Failure", add_duration=True, add_error=True)

    def submit_command_error(self):
        if self.__dll is None:
            return

        _, e, _ = sys.exc_info()

        metricId = self._create_event("ResourceManagement_ParseError")
        self._add_attribute(metricId, "ErrorType", e.__class__.__name__)
        self._submit_event(metricId)

    def set_command_name(self, command_name):
        self._command_name = command_name

    def add_attribute(self, key, value):
        self.__attributes.append((key, value))

    def create_new_event_id(self, event_name):
        if self.__dll is None:
            return -1
        return self._create_event(event_name)

    def add_metric_to_event_by_id(self, metric_id, attribute_name, attribute_value):
        self._add_metric(metric_id, attribute_name, ctypes.c_double(attribute_value))

    def submit_event_by_id(self, metric_id):
        self._submit_event(metric_id)
