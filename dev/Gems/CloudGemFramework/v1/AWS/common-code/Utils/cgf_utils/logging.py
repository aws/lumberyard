
from __future__ import print_function
import sys


class CloudCanvasLogger(object):
    instance = None

    class LogInitializer:
        def __call__(self, *args, **kwargs):
            CloudCanvasLogger.instance = None
            output = None
            if not "output" in kwargs:
                raise Exception("Cannot initialize CloudCanvasLogger, missing \"output\" arg")
            if not "aws_request_id" in kwargs:
                raise Exception("Cannot initialize CloudCanvasLogger, missing \"aws_request_id\" arg")

            output = kwargs["output"]
            logger = CloudCanvasLogger.instance = CloudCanvasLogger(
                kwargs["aws_request_id"], output)
            sys.stdout = logger

    class LogOverrider:
        def __call__(self, *args, **kwargs):
            if not "aws_request_id" in kwargs:
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

    def write(self, message):
        self.Log(message)

    def override_call_id(self, call_id):
        self.call_id = call_id
