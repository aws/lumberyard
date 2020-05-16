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

import json
import logging
import uuid
from threading import Timer, Lock, Thread


class _shadowRequestToken:

    URN_PREFIX_LENGTH = 9

    def getNextToken(self):
        return uuid.uuid4().urn[self.URN_PREFIX_LENGTH:]  # We only need the uuid digits, not the urn prefix


class _basicJSONParser:

    def setString(self, srcString):
        self._rawString = srcString
        self._dictionObject = None

    def regenerateString(self):
        return json.dumps(self._dictionaryObject)

    def getAttributeValue(self, srcAttributeKey):
        return self._dictionaryObject.get(srcAttributeKey)

    def setAttributeValue(self, srcAttributeKey, srcAttributeValue):
        self._dictionaryObject[srcAttributeKey] = srcAttributeValue

    def validateJSON(self):
        try:
            self._dictionaryObject = json.loads(self._rawString)
        except ValueError:
            return False
        return True


class deviceShadow:
    _logger = logging.getLogger(__name__)

    def __init__(self, srcShadowName, srcIsPersistentSubscribe, srcShadowManager):
        """

        The class that denotes a local/client-side device shadow instance.

        Users can perform shadow operations on this instance to retrieve and modify the 
        corresponding shadow JSON document in AWS IoT Cloud. The following shadow operations 
        are available:

        - Get
        
        - Update

        - Delete

        - Listen on delta

        - Cancel listening on delta

        This is returned from :code:`AWSIoTPythonSDK.MQTTLib.AWSIoTMQTTShadowClient.createShadowWithName` function call. 
        No need to call directly from user scripts.

        """
        if srcShadowName is None or srcIsPersistentSubscribe is None or srcShadowManager is None:
            raise TypeError("None type inputs detected.")
        self._shadowName = srcShadowName
        # Tool handler
        self._shadowManagerHandler = srcShadowManager
        self._basicJSONParserHandler = _basicJSONParser()
        self._tokenHandler = _shadowRequestToken()
        # Properties
        self._isPersistentSubscribe = srcIsPersistentSubscribe
        self._lastVersionInSync = -1  # -1 means not initialized
        self._isGetSubscribed = False
        self._isUpdateSubscribed = False
        self._isDeleteSubscribed = False
        self._shadowSubscribeCallbackTable = dict()
        self._shadowSubscribeCallbackTable["get"] = None
        self._shadowSubscribeCallbackTable["delete"] = None
        self._shadowSubscribeCallbackTable["update"] = None
        self._shadowSubscribeCallbackTable["delta"] = None
        self._shadowSubscribeStatusTable = dict()
        self._shadowSubscribeStatusTable["get"] = 0
        self._shadowSubscribeStatusTable["delete"] = 0
        self._shadowSubscribeStatusTable["update"] = 0
        self._tokenPool = dict()
        self._dataStructureLock = Lock()

    def _doNonPersistentUnsubscribe(self, currentAction):
        self._shadowManagerHandler.basicShadowUnsubscribe(self._shadowName, currentAction)
        self._logger.info("Unsubscribed to " + currentAction + " accepted/rejected topics for deviceShadow: " + self._shadowName)

    def generalCallback(self, client, userdata, message):
        # In Py3.x, message.payload comes in as a bytes(string)
        # json.loads needs a string input
        with self._dataStructureLock:
            currentTopic = message.topic
            currentAction = self._parseTopicAction(currentTopic)  # get/delete/update/delta
            currentType = self._parseTopicType(currentTopic)  # accepted/rejected/delta
            payloadUTF8String = message.payload.decode('utf-8')
            # get/delete/update: Need to deal with token, timer and unsubscribe
            if currentAction in ["get", "delete", "update"]:
                # Check for token
                self._basicJSONParserHandler.setString(payloadUTF8String)
                if self._basicJSONParserHandler.validateJSON():  # Filter out invalid JSON
                    currentToken = self._basicJSONParserHandler.getAttributeValue(u"clientToken")
                    if currentToken is not None:
                        self._logger.debug("shadow message clientToken: " + currentToken)
                    if currentToken is not None and currentToken in self._tokenPool.keys():  # Filter out JSON without the desired token
                        # Sync local version when it is an accepted response
                        self._logger.debug("Token is in the pool. Type: " + currentType)
                        if currentType == "accepted":
                            incomingVersion = self._basicJSONParserHandler.getAttributeValue(u"version")
                            # If it is get/update accepted response, we need to sync the local version
                            if incomingVersion is not None and incomingVersion > self._lastVersionInSync and currentAction != "delete":
                                self._lastVersionInSync = incomingVersion
                            # If it is a delete accepted, we need to reset the version
                            else:
                                self._lastVersionInSync = -1  # The version will always be synced for the next incoming delta/GU-accepted response
                        # Cancel the timer and clear the token
                        self._tokenPool[currentToken].cancel()
                        del self._tokenPool[currentToken]
                        # Need to unsubscribe?
                        self._shadowSubscribeStatusTable[currentAction] -= 1
                        if not self._isPersistentSubscribe and self._shadowSubscribeStatusTable.get(currentAction) <= 0:
                            self._shadowSubscribeStatusTable[currentAction] = 0
                            processNonPersistentUnsubscribe = Thread(target=self._doNonPersistentUnsubscribe, args=[currentAction])
                            processNonPersistentUnsubscribe.start()
                        # Custom callback
                        if self._shadowSubscribeCallbackTable.get(currentAction) is not None:
                            processCustomCallback = Thread(target=self._shadowSubscribeCallbackTable[currentAction], args=[payloadUTF8String, currentType, currentToken])
                            processCustomCallback.start()
            # delta: Watch for version
            else:
                currentType += "/" + self._parseTopicShadowName(currentTopic)
                # Sync local version
                self._basicJSONParserHandler.setString(payloadUTF8String)
                if self._basicJSONParserHandler.validateJSON():  # Filter out JSON without version
                    incomingVersion = self._basicJSONParserHandler.getAttributeValue(u"version")
                    if incomingVersion is not None and incomingVersion > self._lastVersionInSync:
                        self._lastVersionInSync = incomingVersion
                        # Custom callback
                        if self._shadowSubscribeCallbackTable.get(currentAction) is not None:
                            processCustomCallback = Thread(target=self._shadowSubscribeCallbackTable[currentAction], args=[payloadUTF8String, currentType, None])
                            processCustomCallback.start()

    def _parseTopicAction(self, srcTopic):
        ret = None
        fragments = srcTopic.split('/')
        if fragments[5] == "delta":
            ret = "delta"
        else:
            ret = fragments[4]
        return ret

    def _parseTopicType(self, srcTopic):
        fragments = srcTopic.split('/')
        return fragments[5]

    def _parseTopicShadowName(self, srcTopic):
        fragments = srcTopic.split('/')
        return fragments[2]

    def _timerHandler(self, srcActionName, srcToken):
        with self._dataStructureLock:
            # Don't crash if we try to remove an unknown token
            if srcToken not in self._tokenPool:
                self._logger.warn('Tried to remove non-existent token from pool: %s' % str(srcToken))
                return
            # Remove the token
            del self._tokenPool[srcToken]
            # Need to unsubscribe?
            self._shadowSubscribeStatusTable[srcActionName] -= 1
            if not self._isPersistentSubscribe and self._shadowSubscribeStatusTable.get(srcActionName) <= 0:
                self._shadowSubscribeStatusTable[srcActionName] = 0
                self._shadowManagerHandler.basicShadowUnsubscribe(self._shadowName, srcActionName)
            # Notify time-out issue
            if self._shadowSubscribeCallbackTable.get(srcActionName) is not None:
                self._logger.info("Shadow request with token: " + str(srcToken) + " has timed out.")
                self._shadowSubscribeCallbackTable[srcActionName]("REQUEST TIME OUT", "timeout", srcToken)

    def shadowGet(self, srcCallback, srcTimeout):
        """
        **Description**

        Retrieve the device shadow JSON document from AWS IoT by publishing an empty JSON document to the 
        corresponding shadow topics. Shadow response topics will be subscribed to receive responses from 
        AWS IoT regarding the result of the get operation. Retrieved shadow JSON document will be available 
        in the registered callback. If no response is received within the provided timeout, a timeout 
        notification will be passed into the registered callback.

        **Syntax**

        .. code:: python

          # Retrieve the shadow JSON document from AWS IoT, with a timeout set to 5 seconds
          BotShadow.shadowGet(customCallback, 5)

        **Parameters**

        *srcCallback* - Function to be called when the response for this shadow request comes back. Should 
        be in form :code:`customCallback(payload, responseStatus, token)`, where :code:`payload` is the 
        JSON document returned, :code:`responseStatus` indicates whether the request has been accepted, 
        rejected or is a delta message, :code:`token` is the token used for tracing in this request.

        *srcTimeout* - Timeout to determine whether the request is invalid. When a request gets timeout, 
        a timeout notification will be generated and put into the registered callback to notify users.

        **Returns**

        The token used for tracing in this shadow request.

        """
        with self._dataStructureLock:
            # Update callback data structure
            self._shadowSubscribeCallbackTable["get"] = srcCallback
            # Update number of pending feedback
            self._shadowSubscribeStatusTable["get"] += 1
            # clientToken
            currentToken = self._tokenHandler.getNextToken()
            self._tokenPool[currentToken] = Timer(srcTimeout, self._timerHandler, ["get", currentToken])
            self._basicJSONParserHandler.setString("{}")
            self._basicJSONParserHandler.validateJSON()
            self._basicJSONParserHandler.setAttributeValue("clientToken", currentToken)
            currentPayload = self._basicJSONParserHandler.regenerateString()
        # Two subscriptions
        if not self._isPersistentSubscribe or not self._isGetSubscribed:
            self._shadowManagerHandler.basicShadowSubscribe(self._shadowName, "get", self.generalCallback)
            self._isGetSubscribed = True
            self._logger.info("Subscribed to get accepted/rejected topics for deviceShadow: " + self._shadowName)
        # One publish
        self._shadowManagerHandler.basicShadowPublish(self._shadowName, "get", currentPayload)
        # Start the timer
        self._tokenPool[currentToken].start()
        return currentToken

    def shadowDelete(self, srcCallback, srcTimeout):
        """
        **Description**

        Delete the device shadow from AWS IoT by publishing an empty JSON document to the corresponding 
        shadow topics. Shadow response topics will be subscribed to receive responses from AWS IoT 
        regarding the result of the get operation. Responses will be available in the registered callback. 
        If no response is received within the provided timeout, a timeout notification will be passed into 
        the registered callback.

        **Syntax**

        .. code:: python

          # Delete the device shadow from AWS IoT, with a timeout set to 5 seconds
          BotShadow.shadowDelete(customCallback, 5)

        **Parameters**

        *srcCallback* - Function to be called when the response for this shadow request comes back. Should 
        be in form :code:`customCallback(payload, responseStatus, token)`, where :code:`payload` is the 
        JSON document returned, :code:`responseStatus` indicates whether the request has been accepted, 
        rejected or is a delta message, :code:`token` is the token used for tracing in this request.

        *srcTimeout* - Timeout to determine whether the request is invalid. When a request gets timeout, 
        a timeout notification will be generated and put into the registered callback to notify users.

        **Returns**

        The token used for tracing in this shadow request.

        """
        with self._dataStructureLock:
            # Update callback data structure
            self._shadowSubscribeCallbackTable["delete"] = srcCallback
            # Update number of pending feedback
            self._shadowSubscribeStatusTable["delete"] += 1
            # clientToken
            currentToken = self._tokenHandler.getNextToken()
            self._tokenPool[currentToken] = Timer(srcTimeout, self._timerHandler, ["delete", currentToken])
            self._basicJSONParserHandler.setString("{}")
            self._basicJSONParserHandler.validateJSON()
            self._basicJSONParserHandler.setAttributeValue("clientToken", currentToken)
            currentPayload = self._basicJSONParserHandler.regenerateString()
        # Two subscriptions
        if not self._isPersistentSubscribe or not self._isDeleteSubscribed:
            self._shadowManagerHandler.basicShadowSubscribe(self._shadowName, "delete", self.generalCallback)
            self._isDeleteSubscribed = True
            self._logger.info("Subscribed to delete accepted/rejected topics for deviceShadow: " + self._shadowName)
        # One publish
        self._shadowManagerHandler.basicShadowPublish(self._shadowName, "delete", currentPayload)
        # Start the timer
        self._tokenPool[currentToken].start()
        return currentToken

    def shadowUpdate(self, srcJSONPayload, srcCallback, srcTimeout):
        """
        **Description**

        Update the device shadow JSON document string from AWS IoT by publishing the provided JSON 
        document to the corresponding shadow topics. Shadow response topics will be subscribed to 
        receive responses from AWS IoT regarding the result of the get operation. Response will be 
        available in the registered callback. If no response is received within the provided timeout, 
        a timeout notification will be passed into the registered callback.

        **Syntax**

        .. code:: python

          # Update the shadow JSON document from AWS IoT, with a timeout set to 5 seconds
          BotShadow.shadowUpdate(newShadowJSONDocumentString, customCallback, 5)

        **Parameters**

        *srcJSONPayload* - JSON document string used to update shadow JSON document in AWS IoT.

        *srcCallback* - Function to be called when the response for this shadow request comes back. Should 
        be in form :code:`customCallback(payload, responseStatus, token)`, where :code:`payload` is the 
        JSON document returned, :code:`responseStatus` indicates whether the request has been accepted, 
        rejected or is a delta message, :code:`token` is the token used for tracing in this request.

        *srcTimeout* - Timeout to determine whether the request is invalid. When a request gets timeout, 
        a timeout notification will be generated and put into the registered callback to notify users.

        **Returns**

        The token used for tracing in this shadow request.

        """
        # Validate JSON
        self._basicJSONParserHandler.setString(srcJSONPayload)
        if self._basicJSONParserHandler.validateJSON():
            with self._dataStructureLock:
                # clientToken
                currentToken = self._tokenHandler.getNextToken()
                self._tokenPool[currentToken] = Timer(srcTimeout, self._timerHandler, ["update", currentToken])
                self._basicJSONParserHandler.setAttributeValue("clientToken", currentToken)
                JSONPayloadWithToken = self._basicJSONParserHandler.regenerateString()
                # Update callback data structure
                self._shadowSubscribeCallbackTable["update"] = srcCallback
                # Update number of pending feedback
                self._shadowSubscribeStatusTable["update"] += 1
            # Two subscriptions
            if not self._isPersistentSubscribe or not self._isUpdateSubscribed:
                self._shadowManagerHandler.basicShadowSubscribe(self._shadowName, "update", self.generalCallback)
                self._isUpdateSubscribed = True
                self._logger.info("Subscribed to update accepted/rejected topics for deviceShadow: " + self._shadowName)
            # One publish
            self._shadowManagerHandler.basicShadowPublish(self._shadowName, "update", JSONPayloadWithToken)
            # Start the timer
            self._tokenPool[currentToken].start()
        else:
            raise ValueError("Invalid JSON file.")
        return currentToken

    def shadowRegisterDeltaCallback(self, srcCallback):
        """
        **Description**

        Listen on delta topics for this device shadow by subscribing to delta topics. Whenever there 
        is a difference between the desired and reported state, the registered callback will be called 
        and the delta payload will be available in the callback.

        **Syntax**

        .. code:: python

          # Listen on delta topics for BotShadow
          BotShadow.shadowRegisterDeltaCallback(customCallback)

        **Parameters**

        *srcCallback* - Function to be called when the response for this shadow request comes back. Should 
        be in form :code:`customCallback(payload, responseStatus, token)`, where :code:`payload` is the 
        JSON document returned, :code:`responseStatus` indicates whether the request has been accepted, 
        rejected or is a delta message, :code:`token` is the token used for tracing in this request.

        **Returns**

        None

        """
        with self._dataStructureLock:
            # Update callback data structure
            self._shadowSubscribeCallbackTable["delta"] = srcCallback
        # One subscription
        self._shadowManagerHandler.basicShadowSubscribe(self._shadowName, "delta", self.generalCallback)
        self._logger.info("Subscribed to delta topic for deviceShadow: " + self._shadowName)

    def shadowUnregisterDeltaCallback(self):
        """
        **Description**

        Cancel listening on delta topics for this device shadow by unsubscribing to delta topics. There will 
        be no delta messages received after this API call even though there is a difference between the 
        desired and reported state.

        **Syntax**

        .. code:: python

          # Cancel listening on delta topics for BotShadow
          BotShadow.shadowUnregisterDeltaCallback()

        **Parameters**

        None

        **Returns**

        None

        """
        with self._dataStructureLock:
            # Update callback data structure
            del self._shadowSubscribeCallbackTable["delta"]
        # One unsubscription
        self._shadowManagerHandler.basicShadowUnsubscribe(self._shadowName, "delta")
        self._logger.info("Unsubscribed to delta topics for deviceShadow: " + self._shadowName)
