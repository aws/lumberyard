
from __future__ import print_function
import sys


class CloudCanvasLogger(object):
    instance = None

    class LogInitializer:
        def __init__(self):
            pass

        def __call__(self, *args, **kwargs):
            CloudCanvasLogger.instance = None
            if "output" not in kwargs:
                raise Exception("Cannot initialize CloudCanvasLogger, missing \"output\" arg")
            if "aws_request_id" not in kwargs:
                raise Exception("Cannot initialize CloudCanvasLogger, missing \"aws_request_id\" arg")

            output = kwargs["output"]
            logger = CloudCanvasLogger.instance = CloudCanvasLogger(
                kwargs["aws_request_id"], output)
            sys.stdout = logger

    class LogOverrider:
        def __init__(self):
            pass

        def __call__(self, *args, **kwargs):
            if "aws_request_id" not in kwargs:
                raise Exception(
                    "Cannot initialize CloudCanvasLogger, missing \"aws_request_id\" arg")
            CloudCanvasLogger.instance.override_call_id(kwargs["aws_request_id"])

    init_instance = LogInitializer()
    override_id = LogOverrider()

    def __init__(self, call_id, output):
        self.call_id = call_id
        self.output = output

    def Log(self, message):
        if not message.isspace():
            self.output.write("Call ({}): {}\n".format(self.call_id, message))

    def flush(self):
        # See if output object has flush command
        if self.output:
            flush_op = getattr(self.output, "flush", None)
            if callable(flush_op):
                flush_op()

    def write(self, message):
        self.Log(message)

    def override_call_id(self, call_id):
        self.call_id = call_id
