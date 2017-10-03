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
import Queue
import threading
import time
import traceback
import logging
import os
import socket
import sys

BUILD_METRICS_FILENAME = 'build_metrics.txt'
LOG_FILE_NAME = 'build_metrics_reporter.log'

MetricUnit = \
{
    'Seconds':'Seconds',
    'Milliseconds':'Milliseconds',
    'Microseconds':'Microseconds',
    'Bytes':'Bytes',
    'Kilobytes':'Kilobytes',
    'Megabytes':'Megabytes',
    'Gigabytes':'Gigabytes',
    'Terabytes':'Terabytes',
    'Bits':'Bits',
    'Kilobits':'Kilobits',
    'Megabits':'Megabits',
    'Gigabits':'Gigabits',
    'Terabits':'Terabits',
    'Percent':'Percent',
    'Count':'Count',
    'BytesPerSecond':'Bytes/Second',
    'KilobytesPerSecond':'Kilobytes/Second',
    'MegabytesPerSecond':'Megabytes/Second',
    'GigabytesPerSecond':'Gigabytes/Second',
    'TerabytesPerSecond':'Terabytes/Second',
    'BitsPerSecond':'Bits/Second',
    'KilobitsPerSecond':'Kilobits/Second',
    'MegabitsPerSecond':'Megabits/Second',
    'GigabitsPerSecond':'Gigabits/Second',
    'TerabitsPerSecond':'Terabits/Second',
    'CountPerSecond':'Count/Second',
    'Unitless':'None'
}

def datetime_to_iso(dt):
    """Given a datetime object, convert it to an ISO-8601 formatted string."""
    return dt.strftime('%Y-%m-%dT%H:%M:%S.%f')


class MetricDatum(object):
    """Represents a single metric data point for reporting."""

    def __init__(self, name, units, timestamp=None):
        self.dimensions = {}
        self.name = name
        self.timestamp = timestamp
        if self.timestamp is None:
            self.timestamp = datetime.datetime.utcnow()
        self.units = units
        self.value = 0.0
        self.additional_metadata = {}

    def __str__(self):
        return json.dumps(self.to_api_format())

    def to_api_format(self):
        dimensions = []
        for key in self.dimensions:
            dimensions.append({'Name': key, 'Value': self.dimensions[key]})

        json_val = {
            'MetricName': self.name,
            'Dimensions': dimensions,
            'Timestamp': datetime_to_iso(self.timestamp),
            'Value': float(self.value),
            'Unit': self.units if self.units is not None else MetricUnit['Unitless'],
            'Metadata': self.additional_metadata
        }

        return json_val


class MetricReporter(object):
    MAX_DIMENSIONS_PER_METRIC = 10
    MAX_DATUM_COUNT_PER_REQUEST = 20
    METRIC_QUEUE_TIMEOUT_SECS = 1
    SHUTDOWN_TIMEOUT_SECS = 5

    def __init__(self, namespace, async=True):
        """Create a new Metric Reporter.

        If async=True (the default), the reporter will not transmit
        anything until the 'start' method is called.  start() will cause a background thread to be spun up that
        will handle transmitting metrics.

        If async=False, metrics will queue up in memory and will not be transmitted until flush() is called."""

        self.namespace = namespace
        self.global_dimensions = {}
        self.async = async
        self.project_spec = None
        self.metrics_namespace = None
        self.use_incredibuild = False

        self.output_directories = []

        self.additional_metric_metadata = {}
        self._last_transmit_time = None
        self._pending_metrics_queue = Queue.Queue()
        self._running = False
        self._metric_send_thread = None
        self._pending_condition = threading.Condition()
        self._force_flush = False

        self._logger = logging.getLogger('MetricReporter')

        # truncate the log file, eventually we need to send the logs to cloudwatch logs
        with open(LOG_FILE_NAME, 'w'):
            pass

        fileHdlr = logging.FileHandler(LOG_FILE_NAME)

        formatter = logging.Formatter('%(asctime)s %(levelname)s %(message)s')
        fileHdlr.setFormatter(formatter)

        self._logger.addHandler(fileHdlr)
        self._logger.setLevel(logging.DEBUG)

        self._logger.info('Created MetricReporter.')

        self.command_line = sys.argv

    def _wake_up_background_thread(self):
        """Signal the background thread to wake up and check the metric queue."""
        if not self.async:
            return

        self._pending_condition.acquire()
        try:
            self._pending_condition.notify()
        finally:
            self._pending_condition.release()

    def parse_command_line(self, command_line):
        self._logger.info('Parsing command line...')

        for index, arg in enumerate(command_line):
            if arg == '-p' or arg == '--project-spec':
                if index+1 < len(command_line):
                    self.project_spec = command_line[index+1]
            elif arg == '--metrics-namespace':
                if index+1 < len(command_line):
                    self.metrics_namespace = command_line.pop(index+1)
            elif arg.lower() == '--use-incredibuild=true':
                self.use_incredibuild=True

        #strip the metrics parameters from argv so WAF doesn't complain about it later
        try:
            command_line.remove('--metrics-namespace')
        except ValueError: # if for some reason this isn't in here we don't want the process to end, it's fine to move on
            pass
        try:
            command_line.remove('--enable-build-metrics')
        except ValueError:
            pass

        if self.metrics_namespace is None:
            return False

        return True

    def set_build_command(self, cmd):
        self.additional_metric_metadata['build_command'] = cmd

    def start(self):
        """Start the background metrics thread. Has no effect is async=False or if
        start() has already been called."""

        self._logger.info('Starting MetricReporter.')

        if not self.async or self._running:
            return

        self._running = True

        self.additional_metric_metadata['host_name'] = socket.gethostname()
        self.additional_metric_metadata['metrics_namespace'] = str(self.metrics_namespace)
        self.additional_metric_metadata['project_spec'] = str(self.project_spec)
        self.additional_metric_metadata['use_incredibuild'] = str(self.use_incredibuild)

        # this gets replaced later
        self.additional_metric_metadata['build_result'] = "#BUILD_RESULT#"

        build_id = os.getenv('BUILD_ID')
        command_id = os.getenv('COMMAND_ID')

        if build_id is None:
            self._logger.error('No build id is set! These metrics will be lost!')
        else:
            self.additional_metric_metadata['build_id'] = build_id

        if command_id is None:
            self._logger.error('No command is is set! These metrics will be lost!')
        else:
            self.additional_metric_metadata['command_id'] = command_id

        self._metric_send_thread = threading.Thread(target=self._metric_loop)
        self._metric_send_thread.daemon = True
        self._metric_send_thread.start()

    def stop(self, gather_output_sizes=False, wait_for_empty_queue=False):
        """Stop the background metrics thread.

        If 'wait_for_empty_queue' is set to True, calling thread will block until all metrics have been
        transmitted or shutdown times out.

        Has no effect if async=False or if background thread is not running."""
        if not self.async or not self._running:
            return

        # setting async to false here will make the flush call later on dump the queue immediately instead of trying
        # to wake up the background thread
        self.async = False

        if gather_output_sizes:
            self.gather_output_sizes()

        self._running = False

        self.flush()

        if wait_for_empty_queue:
            self._logger.info('waiting for empty queue')
            self._metric_send_thread.join(timeout=MetricReporter.SHUTDOWN_TIMEOUT_SECS)

        for handler in list(self._logger.handlers):
            handler.close()
            self._logger.removeHandler(handler)

    def flush(self):
        """If async=True, wakes up background thread and forces a flush of any stored metrics.

        If async=False, this call will block on a synchronous flush of metrics to file."""
        if self.async:
            self._force_flush = True
            self._wake_up_background_thread()
        else:
            while not self._pending_metrics_queue.empty():
                try:
                    self._write_metrics_to_file()
                except :
                    break

    def create_metric(self, name, units, timestamp=None):
        """
        Creates a metric with the given name, units & timestamp.

        If timestamp is not specified, it will default to the current time.

        This method is preferred to directly creating a MetricDatum object because it will also apply
        global dimensions that have been set.
        """
        metric = MetricDatum(name, units, timestamp)

        for key in self.global_dimensions:
            if len(metric.dimensions) >= MetricReporter.MAX_DIMENSIONS_PER_METRIC:
                self._logger.warning('Too many dimensions for %s metric: %')
                break
            metric.dimensions[key] = self.global_dimensions[key]

        metric.additional_metadata = self.additional_metric_metadata

        return metric

    def put_metric(self, metric):
        """
        Put a new metric on the queue to be transmitted.

        If the pending metrics queue is full, the metric will just be ignored and an error printed.
        """
        if not self._running:
            return

        try:
            self._logger.info('MetricsReporter:Metric submitted: %s' % metric.name)
            self._pending_metrics_queue.put_nowait(metric)
        except Queue.Full:
            self._logger.error('Metrics queue is full.  Ignoring new metric named "%s"' % metric.name)

        if self.async:
            self._wake_up_background_thread()

    def add_output_directory(self, out_dir):
        self.output_directories.append(out_dir)

    def gather_output_sizes(self):
        """
        Walk through each output directory and determine it's size, adding them together to determine the final
        overall size of the build
        """

        if len(self.output_directories) == 0:
            return

        self._logger.info('gathering output sizes...')
        size_in_bytes = 0
        for dir in self.output_directories:
            size =  sum( os.path.getsize(os.path.join(dirpath,filename)) for dirpath, dirnames, filenames in os.walk( dir ) for filename in filenames )
            size_in_bytes += size

        submit_metric(self, 'build_size', MetricUnit['Bytes'], size_in_bytes)
        self.flush()


    def _write_metrics_to_file(self):
        metrics_to_write = []
        while not self._pending_metrics_queue.empty() and len(metrics_to_write) < MetricReporter.MAX_DATUM_COUNT_PER_REQUEST:
            try:
                metric = self._pending_metrics_queue.get(True, MetricReporter.METRIC_QUEUE_TIMEOUT_SECS)
                if metric:
                        metrics_to_write.append(metric.to_api_format())
            except Queue.Empty:
                pass

        self._logger.info('Writing %s metrics to file' % str(len(metrics_to_write)))

        if metrics_to_write:
            with open(BUILD_METRICS_FILENAME, 'a') as f:
                for metric in metrics_to_write:
                    self._logger.info('Writing metric %s' % str(metric))
                    f.write(str(metric))
                    f.write('\n')

    def _metric_loop(self):
        """
        Function for background daemon thread (only used if async=True) to consistently loop and write metrics to file.
        """
        if not self.async:
            return

        if self._last_transmit_time is None:
            self._last_transmit_time = time.time()

        self._logger.info('Background metrics thread starting...')

        while self._running:
            try:
                self._pending_condition.acquire()
                try:
                    while (self._pending_metrics_queue.qsize() < MetricReporter.MAX_DATUM_COUNT_PER_REQUEST and
                           time.time()-self._last_transmit_time < MetricReporter.METRIC_QUEUE_TIMEOUT_SECS and
                           self._running and
                           not self._force_flush):
                        self._pending_condition.wait(timeout=MetricReporter.METRIC_QUEUE_TIMEOUT_SECS)
                finally:
                    self._pending_condition.release()

                self._force_flush = False
                self._last_transmit_time = time.time()

                if self._pending_metrics_queue.empty():
                    continue

                self._write_metrics_to_file()

            except Exception as e:
                traceback.print_exc()
                self._logger.error('Error in MetricsReporter thread: %s' % str(e))
                time.sleep(MetricReporter.METRIC_QUEUE_TIMEOUT_SECS)

        self.flush()

        self._logger.info('Background metrics thread terminating...')


def submit_metric(recorder, name, units, value, dimensions=None, timestamp=None):
    """
    This is just a convenience method that creates a MetricDatum,
    populates it with a value and adds it to the pending queue on the supplied MetricRecorder.
    """
    metric_datum = recorder.create_metric(name, units, timestamp)
    metric_datum.value = value
    if dimensions is None:
        dimensions = {}
    metric_datum.dimensions = dimensions
    recorder.put_metric(metric_datum)


def submit_build_metric(name, units, value, dimensions, disable_dimension_unwrap=False, disable_no_dimension=False):
    dim_list = []
    if disable_dimension_unwrap:
        dim_list.append(dimensions.copy())
    else:
        while dimensions:
            dim_list.append(dimensions.copy())
            dimensions.popitem()

    if not disable_no_dimension:
        dim_list.append({})

    dim_list.reverse()

    for dimension_dict in dim_list:
        submit_metric(metric_reporter, name, units, value, dimension_dict)

def add_output_directory(out_dir):
    metric_reporter.add_output_directory(out_dir)

def set_build_command(cmd):
    metric_reporter.set_build_command(cmd)

metric_reporter = MetricReporter('WAF_BuildMetrics')

def stop_metric_reporter(gather_output_sizes=False, wait_for_empty_queue=False):
    metric_reporter.stop(gather_output_sizes, gather_output_sizes)

def start_metric_reporter():
    metric_reporter.start()
