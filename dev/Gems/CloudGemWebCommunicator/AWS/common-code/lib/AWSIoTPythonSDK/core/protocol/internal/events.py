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

class EventTypes(object):
    CONNACK = 0
    DISCONNECT = 1
    PUBACK = 2
    SUBACK = 3
    UNSUBACK = 4
    MESSAGE = 5


class FixedEventMids(object):
    CONNACK_MID = "CONNECTED"
    DISCONNECT_MID = "DISCONNECTED"
    MESSAGE_MID = "MESSAGE"
    QUEUED_MID = "QUEUED"
