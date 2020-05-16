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

class RequestTypes(object):
    CONNECT = 0
    DISCONNECT = 1
    PUBLISH = 2
    SUBSCRIBE = 3
    UNSUBSCRIBE = 4

class QueueableRequest(object):

    def __init__(self, type, data):
        self.type = type
        self.data = data  # Can be a tuple
