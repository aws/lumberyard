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

from errors import HandledError
import os
import ctypes
import inspect
import platform
import sys
import linecache
import traceback
import time
import string
import atexit

def get_lymetrics_library_path():
    # assumes this file is ...\dev\Gems\CloudGemFramework\v?\ResourceManager\resource_manager\metrics.py
    # we want ...\dev\Tools\InternalSDKs\LyMetrics\bin
    path = os.path.abspath(os.path.join(__file__, '..', '..', '..', '..', '..', '..'))
    path = os.path.join(path, 'Tools', 'InternalSDKs', 'LyMetrics', 'bin')
    if platform.system() == "Windows":
        path = os.path.join(path, 'windows', 'intel64')
        for build in ['Release', 'Debug']:
            for vs in ['vs2017', 'vs2015', 'vs2013']:
                dll = os.path.join(path, vs, build, 'LyMetricsProducer_python.dll')
                if os.path.exists(dll):
                    return dll
    return None

def load_lymetrics_library():

    dll_path = get_lymetrics_library_path()

    if dll_path is None:
        dll = None
    else:
        try:
            dll = ctypes.cdll.LoadLibrary(dll_path)
            dll.LyMetrics_CreateEvent.restype = ctypes.c_uint64
            dll.LyMetrics_Initialize("lmbraws", 2, True, None, None)
        except:
            dll = None

    return dll

lymetrics_dll = None

def get_lymetrics_library():
    global lymetrics_dll
    if lymetrics_dll is None:
        lymetrics_dll = load_lymetrics_library()
    return lymetrics_dll

@atexit.register
def unload_lymetrics_library():
    global lymetrics_dll
    if lymetrics_dll is not None:
        lymetrics_dll.LyMetrics_Shutdown()

class MetricsContext(object):

    '''A wrapper for the LyMetricsProducer.dll library'''

    def __init__(self, origin):
        self.__dll = get_lymetrics_library()
        self.__startTime = time.clock()
        self.__origin = origin
        self.__attributes = []

    def __str__(self):
        return 'MetricsWrapper'

    # Internal
    #
    # LyMetrics forwarded interface
    def _create_event(self, name):
        if self.__dll is not None:
            return self.__dll.LyMetrics_CreateEvent(name)
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
            return self.__dll.LyMetrics_AddAttribute(ctypes.c_uint64(id), key, value)
        else:
            return False

    def _add_metric(self, id, key, value):
        if self.__dll is not None:
            return self.__dll.LyMetrics_AddMetric(ctypes.c_uint64(id), key, value)
        else:
            return False

    def _set_context(self, key, value):
        if self.__dll is not None:
            self.__dll.LyMetrics_SetContext(key, value)

    def _clear_context(self, key):
        if self.__dll is not None:
            self.__dll.LyMetrics_ClearContext(key)

    # helper
    def _get_operation_name(self):
        return string.join([word.capitalize() for word in self.commandName.split('-')], "")


    def _submit_command_metric(self, status, addDuration, addError):

        if self.__dll is None:
            return

        metricId = self._create_event("ResourceManagement_Command")
        if metricId == -1:
            print 'ERROR: Cloud not create metrics event.' # don't have access to context.view, this will do...
            return

        self._add_attribute(metricId, "CommandOrigin", self.__origin)
        self._add_attribute(metricId, "Operation", self._get_operation_name())
        self._add_attribute(metricId, "Status", status)

        for attribute in self.__attributes:
            self._add_attribute(metricId, attribute[0], attribute[1])

        if addDuration:
            timeElapsed = time.clock() - self.__startTime
            self._add_metric(metricId, "Duration", ctypes.c_double(timeElapsed))

        if addError:
            _, e, tb = sys.exc_info()
            frames = traceback.extract_tb(tb)
            filename, lineNo, _, _ = frames[len(frames) - 1]
            strippedFilename = os.path.basename(filename)

            self._add_attribute(metricId, "ErrorType", e.__class__.__name__)
            self._add_attribute(metricId, "FileName", strippedFilename)
            self._add_attribute(metricId, "LineNumber", str(lineNo))

        if not self._submit_event(metricId):
            print 'ERROR: Cloud not submit metrics event.' # don't have access to context.view, this will do...
            return


    # high-level lmbr_aws interface
    def submit_attempt(self):
        self._submit_command_metric( "Attempt", addDuration=False, addError=False )


    def submit_success(self):
        self._submit_command_metric( "Success", addDuration=True, addError=False )


    def submit_interrupted(self):
        self._submit_command_metric( "Interrupted", addDuration=True, addError=False )


    def submit_failure(self):
        self._submit_command_metric( "Failure", addDuration=True, addError=True )


    def submit_command_error(self):
        if self.__dll is None:
            return

        _, e, _ = sys.exc_info()

        metricId = self._create_event("ResourceManagement_ParseError")
        self._add_attribute(metricId, "ErrorType", e.__class__.__name__)
        self._submit_event(metricId)


    def set_command_name(self, commandName):
        self.commandName = commandName

    def add_attribute(self, key, value):
        self.__attributes.append((key, value))
