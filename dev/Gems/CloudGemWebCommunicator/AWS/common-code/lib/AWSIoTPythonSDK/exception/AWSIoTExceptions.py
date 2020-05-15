# /*
# * Copyright 2010-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
# *
# * Licensed under the Apache License, Version 2.0 (the "License").
# * You may not use this file except in compliance with the License.
# * A copy of the License is located at
# *
# *  http://aws.amazon.com/apache2.0
# *
# * or in the "license" file accompanying this file. This file is distributed
# * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
# * express or implied. See the License for the specific language governing
# * permissions and limitations under the License.
# */

import AWSIoTPythonSDK.exception.operationTimeoutException as operationTimeoutException
import AWSIoTPythonSDK.exception.operationError as operationError


# Serial Exception
class acceptTimeoutException(Exception):
    def __init__(self, msg="Accept Timeout"):
        self.message = msg


# MQTT Operation Timeout Exception
class connectTimeoutException(operationTimeoutException.operationTimeoutException):
    def __init__(self, msg="Connect Timeout"):
        self.message = msg


class disconnectTimeoutException(operationTimeoutException.operationTimeoutException):
    def __init__(self, msg="Disconnect Timeout"):
        self.message = msg


class publishTimeoutException(operationTimeoutException.operationTimeoutException):
    def __init__(self, msg="Publish Timeout"):
        self.message = msg


class subscribeTimeoutException(operationTimeoutException.operationTimeoutException):
    def __init__(self, msg="Subscribe Timeout"):
        self.message = msg


class unsubscribeTimeoutException(operationTimeoutException.operationTimeoutException):
    def __init__(self, msg="Unsubscribe Timeout"):
        self.message = msg


# MQTT Operation Error
class connectError(operationError.operationError):
    def __init__(self, errorCode):
        self.message = "Connect Error: " + str(errorCode)


class disconnectError(operationError.operationError):
    def __init__(self, errorCode):
        self.message = "Disconnect Error: " + str(errorCode)


class publishError(operationError.operationError):
    def __init__(self, errorCode):
        self.message = "Publish Error: " + str(errorCode)


class publishQueueFullException(operationError.operationError):
    def __init__(self):
        self.message = "Internal Publish Queue Full"


class publishQueueDisabledException(operationError.operationError):
    def __init__(self):
        self.message = "Offline publish request dropped because queueing is disabled"


class subscribeError(operationError.operationError):
    def __init__(self, errorCode):
        self.message = "Subscribe Error: " + str(errorCode)


class subscribeQueueFullException(operationError.operationError):
    def __init__(self):
        self.message = "Internal Subscribe Queue Full"


class subscribeQueueDisabledException(operationError.operationError):
    def __init__(self):
        self.message = "Offline subscribe request dropped because queueing is disabled"


class unsubscribeError(operationError.operationError):
    def __init__(self, errorCode):
        self.message = "Unsubscribe Error: " + str(errorCode)


class unsubscribeQueueFullException(operationError.operationError):
    def __init__(self):
        self.message = "Internal Unsubscribe Queue Full"


class unsubscribeQueueDisabledException(operationError.operationError):
    def __init__(self):
        self.message = "Offline unsubscribe request dropped because queueing is disabled"


# Websocket Error
class wssNoKeyInEnvironmentError(operationError.operationError):
    def __init__(self):
        self.message = "No AWS_ACCESS_KEY_ID and AWS_SECRET_ACCESS_KEY detected in $ENV."


class wssHandShakeError(operationError.operationError):
    def __init__(self):
        self.message = "Error in WSS handshake."


# Greengrass Discovery Error
class DiscoveryDataNotFoundException(operationError.operationError):
    def __init__(self):
        self.message = "No discovery data found"


class DiscoveryTimeoutException(operationTimeoutException.operationTimeoutException):
    def __init__(self, message="Discovery request timed out"):
        self.message = message


class DiscoveryInvalidRequestException(operationError.operationError):
    def __init__(self):
        self.message = "Invalid discovery request"


class DiscoveryUnauthorizedException(operationError.operationError):
    def __init__(self):
        self.message = "Discovery request not authorized"


class DiscoveryThrottlingException(operationError.operationError):
    def __init__(self):
        self.message = "Too many discovery requests"


class DiscoveryFailure(operationError.operationError):
    def __init__(self, message):
        self.message = message


# Client Error
class ClientError(Exception):
    def __init__(self, message):
        self.message = message
