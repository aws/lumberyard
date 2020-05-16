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

# This class implements the progressive backoff logic for auto-reconnect.
# It manages the reconnect wait time for the current reconnect, controling
# when to increase it and when to reset it.


import re
import sys
import ssl
import errno
import struct
import socket
import base64
import time
import threading
import logging
import os
from datetime import datetime
import hashlib
import hmac
from AWSIoTPythonSDK.exception.AWSIoTExceptions import ClientError
from AWSIoTPythonSDK.exception.AWSIoTExceptions import wssNoKeyInEnvironmentError
from AWSIoTPythonSDK.exception.AWSIoTExceptions import wssHandShakeError
from AWSIoTPythonSDK.core.protocol.internal.defaults import DEFAULT_CONNECT_DISCONNECT_TIMEOUT_SEC
try:
    from urllib.parse import quote  # Python 3+
except ImportError:
    from urllib import quote
# INI config file handling
try:
    from configparser import ConfigParser  # Python 3+
    from configparser import NoOptionError
    from configparser import NoSectionError
except ImportError:
    from ConfigParser import ConfigParser
    from ConfigParser import NoOptionError
    from ConfigParser import NoSectionError


class ProgressiveBackOffCore:
    # Logger
    _logger = logging.getLogger(__name__)

    def __init__(self, srcBaseReconnectTimeSecond=1, srcMaximumReconnectTimeSecond=32, srcMinimumConnectTimeSecond=20):
        # The base reconnection time in seconds, default 1
        self._baseReconnectTimeSecond = srcBaseReconnectTimeSecond
        # The maximum reconnection time in seconds, default 32
        self._maximumReconnectTimeSecond = srcMaximumReconnectTimeSecond
        # The minimum time in milliseconds that a connection must be maintained in order to be considered stable
        # Default 20
        self._minimumConnectTimeSecond = srcMinimumConnectTimeSecond
        # Current backOff time in seconds, init to equal to 0
        self._currentBackoffTimeSecond = 1
        # Handler for timer
        self._resetBackoffTimer = None

    # For custom progressiveBackoff timing configuration
    def configTime(self, srcBaseReconnectTimeSecond, srcMaximumReconnectTimeSecond, srcMinimumConnectTimeSecond):
        if srcBaseReconnectTimeSecond < 0 or srcMaximumReconnectTimeSecond < 0 or srcMinimumConnectTimeSecond < 0:
            self._logger.error("init: Negative time configuration detected.")
            raise ValueError("Negative time configuration detected.")
        if srcBaseReconnectTimeSecond >= srcMinimumConnectTimeSecond:
            self._logger.error("init: Min connect time should be bigger than base reconnect time.")
            raise ValueError("Min connect time should be bigger than base reconnect time.")
        self._baseReconnectTimeSecond = srcBaseReconnectTimeSecond
        self._maximumReconnectTimeSecond = srcMaximumReconnectTimeSecond
        self._minimumConnectTimeSecond = srcMinimumConnectTimeSecond
        self._currentBackoffTimeSecond = 1

    # Block the reconnect logic for _currentBackoffTimeSecond
    # Update the currentBackoffTimeSecond for the next reconnect
    # Cancel the in-waiting timer for resetting backOff time
    # This should get called only when a disconnect/reconnect happens
    def backOff(self):
        self._logger.debug("backOff: current backoff time is: " + str(self._currentBackoffTimeSecond) + " sec.")
        if self._resetBackoffTimer is not None:
            # Cancel the timer
            self._resetBackoffTimer.cancel()
        # Block the reconnect logic
        time.sleep(self._currentBackoffTimeSecond)
        # Update the backoff time
        if self._currentBackoffTimeSecond == 0:
            # This is the first attempt to connect, set it to base
            self._currentBackoffTimeSecond = self._baseReconnectTimeSecond
        else:
            # r_cur = min(2^n*r_base, r_max)
            self._currentBackoffTimeSecond = min(self._maximumReconnectTimeSecond, self._currentBackoffTimeSecond * 2)

    # Start the timer for resetting _currentBackoffTimeSecond
    # Will be cancelled upon calling backOff
    def startStableConnectionTimer(self):
        self._resetBackoffTimer = threading.Timer(self._minimumConnectTimeSecond,
                                                  self._connectionStableThenResetBackoffTime)
        self._resetBackoffTimer.start()

    def stopStableConnectionTimer(self):
        if self._resetBackoffTimer is not None:
            # Cancel the timer
            self._resetBackoffTimer.cancel()

    # Timer callback to reset _currentBackoffTimeSecond
    # If the connection is stable for longer than _minimumConnectTimeSecond,
    # reset the currentBackoffTimeSecond to _baseReconnectTimeSecond
    def _connectionStableThenResetBackoffTime(self):
        self._logger.debug(
            "stableConnection: Resetting the backoff time to: " + str(self._baseReconnectTimeSecond) + " sec.")
        self._currentBackoffTimeSecond = self._baseReconnectTimeSecond


class SigV4Core:

    _logger = logging.getLogger(__name__)

    def __init__(self):
        self._aws_access_key_id = ""
        self._aws_secret_access_key = ""
        self._aws_session_token = ""
        self._credentialConfigFilePath = "~/.aws/credentials"

    def setIAMCredentials(self, srcAWSAccessKeyID, srcAWSSecretAccessKey, srcAWSSessionToken):
        self._aws_access_key_id = srcAWSAccessKeyID
        self._aws_secret_access_key = srcAWSSecretAccessKey
        self._aws_session_token = srcAWSSessionToken

    def _createAmazonDate(self):
        # Returned as a unicode string in Py3.x
        amazonDate = []
        currentTime = datetime.utcnow()
        YMDHMS = currentTime.strftime('%Y%m%dT%H%M%SZ')
        YMD = YMDHMS[0:YMDHMS.index('T')]
        amazonDate.append(YMD)
        amazonDate.append(YMDHMS)
        return amazonDate

    def _sign(self, key, message):
        # Returned as a utf-8 byte string in Py3.x
        return hmac.new(key, message.encode('utf-8'), hashlib.sha256).digest()

    def _getSignatureKey(self, key, dateStamp, regionName, serviceName):
        # Returned as a utf-8 byte string in Py3.x
        kDate = self._sign(('AWS4' + key).encode('utf-8'), dateStamp)
        kRegion = self._sign(kDate, regionName)
        kService = self._sign(kRegion, serviceName)
        kSigning = self._sign(kService, 'aws4_request')
        return kSigning

    def _checkIAMCredentials(self):
        # Check custom config
        ret = self._checkKeyInCustomConfig()
        # Check environment variables
        if not ret:
            ret = self._checkKeyInEnv()
        # Check files
        if not ret:
            ret = self._checkKeyInFiles()
        # All credentials returned as unicode strings in Py3.x
        return ret

    def _checkKeyInEnv(self):
        ret = dict()
        self._aws_access_key_id = os.environ.get('AWS_ACCESS_KEY_ID')
        self._aws_secret_access_key = os.environ.get('AWS_SECRET_ACCESS_KEY')
        self._aws_session_token = os.environ.get('AWS_SESSION_TOKEN')
        if self._aws_access_key_id is not None and self._aws_secret_access_key is not None:
            ret["aws_access_key_id"] = self._aws_access_key_id
            ret["aws_secret_access_key"] = self._aws_secret_access_key
        # We do not necessarily need session token...
        if self._aws_session_token is not None:
            ret["aws_session_token"] = self._aws_session_token
        self._logger.debug("IAM credentials from env var.")
        return ret

    def _checkKeyInINIDefault(self, srcConfigParser, sectionName):
        ret = dict()
        # Check aws_access_key_id and aws_secret_access_key
        try:
            ret["aws_access_key_id"] = srcConfigParser.get(sectionName, "aws_access_key_id")
            ret["aws_secret_access_key"] = srcConfigParser.get(sectionName, "aws_secret_access_key")
        except NoOptionError:
            self._logger.warn("Cannot find IAM keyID/secretKey in credential file.")
        # We do not continue searching if we cannot even get IAM id/secret right
        if len(ret) == 2:
            # Check aws_session_token, optional
            try:
                ret["aws_session_token"] = srcConfigParser.get(sectionName, "aws_session_token")
            except NoOptionError:
                self._logger.debug("No AWS Session Token found.")
        return ret

    def _checkKeyInFiles(self):
        credentialFile = None
        credentialConfig = None
        ret = dict()
        # Should be compatible with aws cli default credential configuration
        # *NIX/Windows
        try:
            # See if we get the file
            credentialConfig = ConfigParser()
            credentialFilePath = os.path.expanduser(self._credentialConfigFilePath)  # Is it compatible with windows? \/
            credentialConfig.read(credentialFilePath)
            # Now we have the file, start looking for credentials...
            # 'default' section
            ret = self._checkKeyInINIDefault(credentialConfig, "default")
            if not ret:
                # 'DEFAULT' section
                ret = self._checkKeyInINIDefault(credentialConfig, "DEFAULT")
            self._logger.debug("IAM credentials from file.")
        except IOError:
            self._logger.debug("No IAM credential configuration file in " + credentialFilePath)
        except NoSectionError:
            self._logger.error("Cannot find IAM 'default' section.")
        return ret

    def _checkKeyInCustomConfig(self):
        ret = dict()
        if self._aws_access_key_id != "" and self._aws_secret_access_key != "":
            ret["aws_access_key_id"] = self._aws_access_key_id
            ret["aws_secret_access_key"] = self._aws_secret_access_key
        # We do not necessarily need session token...
        if self._aws_session_token != "":
            ret["aws_session_token"] = self._aws_session_token
        self._logger.debug("IAM credentials from custom config.")
        return ret

    def createWebsocketEndpoint(self, host, port, region, method, awsServiceName, path):
        # Return the endpoint as unicode string in 3.x
        # Gather all the facts
        amazonDate = self._createAmazonDate()
        amazonDateSimple = amazonDate[0]  # Unicode in 3.x
        amazonDateComplex = amazonDate[1]  # Unicode in 3.x
        allKeys = self._checkIAMCredentials()  # Unicode in 3.x
        if not self._hasCredentialsNecessaryForWebsocket(allKeys):
            raise wssNoKeyInEnvironmentError()
        else:
            # Because of self._hasCredentialsNecessaryForWebsocket(...), keyID and secretKey should not be None from here
            keyID = allKeys["aws_access_key_id"]
            secretKey = allKeys["aws_secret_access_key"]
            # amazonDateSimple and amazonDateComplex are guaranteed not to be None
            queryParameters = "X-Amz-Algorithm=AWS4-HMAC-SHA256" + \
                "&X-Amz-Credential=" + keyID + "%2F" + amazonDateSimple + "%2F" + region + "%2F" + awsServiceName + "%2Faws4_request" + \
                "&X-Amz-Date=" + amazonDateComplex + \
                "&X-Amz-Expires=86400" + \
                "&X-Amz-SignedHeaders=host"  # Unicode in 3.x
            hashedPayload = hashlib.sha256(str("").encode('utf-8')).hexdigest()  # Unicode in 3.x
            # Create the string to sign
            signedHeaders = "host"
            canonicalHeaders = "host:" + host + "\n"
            canonicalRequest = method + "\n" + path + "\n" + queryParameters + "\n" + canonicalHeaders + "\n" + signedHeaders + "\n" + hashedPayload  # Unicode in 3.x
            hashedCanonicalRequest = hashlib.sha256(str(canonicalRequest).encode('utf-8')).hexdigest()  # Unicoede in 3.x
            stringToSign = "AWS4-HMAC-SHA256\n" + amazonDateComplex + "\n" + amazonDateSimple + "/" + region + "/" + awsServiceName + "/aws4_request\n" + hashedCanonicalRequest  # Unicode in 3.x
            # Sign it
            signingKey = self._getSignatureKey(secretKey, amazonDateSimple, region, awsServiceName)
            signature = hmac.new(signingKey, (stringToSign).encode("utf-8"), hashlib.sha256).hexdigest()
            # generate url
            url = "wss://" + host + ":" + str(port) + path + '?' + queryParameters + "&X-Amz-Signature=" + signature
            # See if we have STS token, if we do, add it
            awsSessionTokenCandidate = allKeys.get("aws_session_token")
            if awsSessionTokenCandidate is not None and len(awsSessionTokenCandidate) != 0:
                aws_session_token = allKeys["aws_session_token"]
                url += "&X-Amz-Security-Token=" + quote(aws_session_token.encode("utf-8"))  # Unicode in 3.x
            self._logger.debug("createWebsocketEndpoint: Websocket URL: " + url)
            return url

    def _hasCredentialsNecessaryForWebsocket(self, allKeys):
        awsAccessKeyIdCandidate = allKeys.get("aws_access_key_id")
        awsSecretAccessKeyCandidate = allKeys.get("aws_secret_access_key")
        # None value is NOT considered as valid entries
        validEntries = awsAccessKeyIdCandidate is not None and awsAccessKeyIdCandidate is not None
        if validEntries:
            # Empty value is NOT considered as valid entries
            validEntries &= (len(awsAccessKeyIdCandidate) != 0 and len(awsSecretAccessKeyCandidate) != 0)
        return validEntries


# This is an internal class that buffers the incoming bytes into an
# internal buffer until it gets the full desired length of bytes.
# At that time, this bufferedReader will be reset.
# *Error handling:
# For retry errors (ssl.SSL_ERROR_WANT_READ, ssl.SSL_ERROR_WANT_WRITE, EAGAIN),
# leave them to the paho _packet_read for further handling (ignored and try
# again when data is available.
# For other errors, leave them to the paho _packet_read for error reporting.


class _BufferedReader:
    _sslSocket = None
    _internalBuffer = None
    _remainedLength = -1
    _bufferingInProgress = False

    def __init__(self, sslSocket):
        self._sslSocket = sslSocket
        self._internalBuffer = bytearray()
        self._bufferingInProgress = False

    def _reset(self):
        self._internalBuffer = bytearray()
        self._remainedLength = -1
        self._bufferingInProgress = False

    def read(self, numberOfBytesToBeBuffered):
        if not self._bufferingInProgress:  # If last read is completed...
            self._remainedLength = numberOfBytesToBeBuffered
            self._bufferingInProgress = True  # Now we start buffering a new length of bytes

        while self._remainedLength > 0:  # Read in a loop, always try to read in the remained length
            # If the data is temporarily not available, socket.error will be raised and catched by paho
            dataChunk = self._sslSocket.read(self._remainedLength)
            # There is a chance where the server terminates the connection without closing the socket.
            # If that happens, let's raise an exception and enter the reconnect flow.
            if not dataChunk:
                raise socket.error(errno.ECONNABORTED, 0)
            self._internalBuffer.extend(dataChunk)  # Buffer the data
            self._remainedLength -= len(dataChunk)  # Update the remained length

        # The requested length of bytes is buffered, recover the context and return it
        # Otherwise error should be raised
        ret = self._internalBuffer
        self._reset()
        return ret  # This should always be bytearray


# This is the internal class that sends requested data out chunk by chunk according
# to the availablity of the socket write operation. If the requested bytes of data
# (after encoding) needs to be sent out in separate socket write operations (most
# probably be interrupted by the error socket.error (errno = ssl.SSL_ERROR_WANT_WRITE).)
# , the write pointer is stored to ensure that the continued bytes will be sent next
# time this function gets called.
# *Error handling:
# For retry errors (ssl.SSL_ERROR_WANT_READ, ssl.SSL_ERROR_WANT_WRITE, EAGAIN),
# leave them to the paho _packet_read for further handling (ignored and try
# again when data is available.
# For other errors, leave them to the paho _packet_read for error reporting.


class _BufferedWriter:
    _sslSocket = None
    _internalBuffer = None
    _writingInProgress = False
    _requestedDataLength = -1

    def __init__(self, sslSocket):
        self._sslSocket = sslSocket
        self._internalBuffer = bytearray()
        self._writingInProgress = False
        self._requestedDataLength = -1

    def _reset(self):
        self._internalBuffer = bytearray()
        self._writingInProgress = False
        self._requestedDataLength = -1

    # Input data for this function needs to be an encoded wss frame
    # Always request for packet[pos=0:] (raw MQTT data)
    def write(self, encodedData, payloadLength):
        # encodedData should always be bytearray
        # Check if we have a frame that is partially sent
        if not self._writingInProgress:
            self._internalBuffer = encodedData
            self._writingInProgress = True
            self._requestedDataLength = payloadLength
        # Now, write as much as we can
        lengthWritten = self._sslSocket.write(self._internalBuffer)
        self._internalBuffer = self._internalBuffer[lengthWritten:]
        # This MQTT packet has been sent out in a wss frame, completely
        if len(self._internalBuffer) == 0:
            ret = self._requestedDataLength
            self._reset()
            return ret
        # This socket write is half-baked...
        else:
            return 0  # Ensure that the 'pos' inside the MQTT packet never moves since we have not finished the transmission of this encoded frame


class SecuredWebSocketCore:
    # Websocket Constants
    _OP_CONTINUATION = 0x0
    _OP_TEXT = 0x1
    _OP_BINARY = 0x2
    _OP_CONNECTION_CLOSE = 0x8
    _OP_PING = 0x9
    _OP_PONG = 0xa
    # Websocket Connect Status
    _WebsocketConnectInit = -1
    _WebsocketDisconnected = 1

    _logger = logging.getLogger(__name__)

    def __init__(self, socket, hostAddress, portNumber, AWSAccessKeyID="", AWSSecretAccessKey="", AWSSessionToken=""):
        self._connectStatus = self._WebsocketConnectInit
        # Handlers
        self._sslSocket = socket
        self._sigV4Handler = self._createSigV4Core()
        self._sigV4Handler.setIAMCredentials(AWSAccessKeyID, AWSSecretAccessKey, AWSSessionToken)
        # Endpoint Info
        self._hostAddress = hostAddress
        self._portNumber = portNumber
        # Section Flags
        self._hasOpByte = False
        self._hasPayloadLengthFirst = False
        self._hasPayloadLengthExtended = False
        self._hasMaskKey = False
        self._hasPayload = False
        # Properties for current websocket frame
        self._isFIN = False
        self._RSVBits = None
        self._opCode = None
        self._needMaskKey = False
        self._payloadLengthBytesLength = 1
        self._payloadLength = 0
        self._maskKey = None
        self._payloadDataBuffer = bytearray()  # Once the whole wss connection is lost, there is no need to keep the buffered payload
        try:
            self._handShake(hostAddress, portNumber)
        except wssNoKeyInEnvironmentError:  # Handle SigV4 signing and websocket handshaking errors
            raise ValueError("No Access Key/KeyID Error")
        except wssHandShakeError:
            raise ValueError("Websocket Handshake Error")
        except ClientError as e:
            raise ValueError(e.message)
        # Now we have a socket with secured websocket...
        self._bufferedReader = _BufferedReader(self._sslSocket)
        self._bufferedWriter = _BufferedWriter(self._sslSocket)

    def _createSigV4Core(self):
        return SigV4Core()

    def _generateMaskKey(self):
        return bytearray(os.urandom(4))
        # os.urandom returns ascii str in 2.x, converted to bytearray
        # os.urandom returns bytes in 3.x, converted to bytearray

    def _reset(self):  # Reset the context for wss frame reception
        # Control info
        self._hasOpByte = False
        self._hasPayloadLengthFirst = False
        self._hasPayloadLengthExtended = False
        self._hasMaskKey = False
        self._hasPayload = False
        # Frame Info
        self._isFIN = False
        self._RSVBits = None
        self._opCode = None
        self._needMaskKey = False
        self._payloadLengthBytesLength = 1
        self._payloadLength = 0
        self._maskKey = None
        # Never reset the payloadData since we might have fragmented MQTT data from the pervious frame

    def _generateWSSKey(self):
        return base64.b64encode(os.urandom(128))  # Bytes

    def _verifyWSSResponse(self, response, clientKey):
        # Check if it is a 101 response
        rawResponse = response.strip().lower()
        if b"101 switching protocols" not in rawResponse or b"upgrade: websocket" not in rawResponse or b"connection: upgrade" not in rawResponse:
            return False
        # Parse out the sec-websocket-accept
        WSSAcceptKeyIndex = response.strip().index(b"sec-websocket-accept: ") + len(b"sec-websocket-accept: ")
        rawSecWebSocketAccept = response.strip()[WSSAcceptKeyIndex:].split(b"\r\n")[0].strip()
        # Verify the WSSAcceptKey
        return self._verifyWSSAcceptKey(rawSecWebSocketAccept, clientKey)

    def _verifyWSSAcceptKey(self, srcAcceptKey, clientKey):
        GUID = b"258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
        verifyServerAcceptKey = base64.b64encode((hashlib.sha1(clientKey + GUID)).digest())  # Bytes
        return srcAcceptKey == verifyServerAcceptKey

    def _handShake(self, hostAddress, portNumber):
        CRLF = "\r\n"
        IOT_ENDPOINT_PATTERN = r"^[0-9a-zA-Z]+(\.ats|-ats)?\.iot\.(.*)\.amazonaws\..*"
        matched = re.compile(IOT_ENDPOINT_PATTERN, re.IGNORECASE).match(hostAddress)
        if not matched:
            raise ClientError("Invalid endpoint pattern for wss: %s" % hostAddress)
        region = matched.group(2)
        signedURL = self._sigV4Handler.createWebsocketEndpoint(hostAddress, portNumber, region, "GET", "iotdata", "/mqtt")
        # Now we got a signedURL
        path = signedURL[signedURL.index("/mqtt"):]
        # Assemble HTTP request headers
        Method = "GET " + path + " HTTP/1.1" + CRLF
        Host = "Host: " + hostAddress + CRLF
        Connection = "Connection: " + "Upgrade" + CRLF
        Upgrade = "Upgrade: " + "websocket" + CRLF
        secWebSocketVersion = "Sec-WebSocket-Version: " + "13" + CRLF
        rawSecWebSocketKey = self._generateWSSKey()  # Bytes
        secWebSocketKey = "sec-websocket-key: " + rawSecWebSocketKey.decode('utf-8') + CRLF  # Should be randomly generated...
        secWebSocketProtocol = "Sec-WebSocket-Protocol: " + "mqttv3.1" + CRLF
        secWebSocketExtensions = "Sec-WebSocket-Extensions: " + "permessage-deflate; client_max_window_bits" + CRLF
        # Send the HTTP request
        # Ensure that we are sending bytes, not by any chance unicode string
        handshakeBytes = Method + Host + Connection + Upgrade + secWebSocketVersion + secWebSocketProtocol + secWebSocketExtensions + secWebSocketKey + CRLF
        handshakeBytes = handshakeBytes.encode('utf-8')
        self._sslSocket.write(handshakeBytes)
        # Read it back (Non-blocking socket)
        timeStart = time.time()
        wssHandshakeResponse = bytearray()
        while len(wssHandshakeResponse) == 0:
            try:
                wssHandshakeResponse += self._sslSocket.read(1024)  # Response is always less than 1024 bytes
            except socket.error as err:
                if err.errno == ssl.SSL_ERROR_WANT_READ or err.errno == ssl.SSL_ERROR_WANT_WRITE:
                    if time.time() - timeStart > self._getTimeoutSec():
                        raise err  # We make sure that reconnect gets retried in Paho upon a wss reconnect response timeout
                else:
                    raise err
        # Verify response
        # Now both wssHandshakeResponse and rawSecWebSocketKey are byte strings
        if not self._verifyWSSResponse(wssHandshakeResponse, rawSecWebSocketKey):
            raise wssHandShakeError()
        else:
            pass

    def _getTimeoutSec(self):
        return DEFAULT_CONNECT_DISCONNECT_TIMEOUT_SEC

    # Used to create a single wss frame
    # Assume that the maximum length of a MQTT packet never exceeds the maximum length
    # for a wss frame. Therefore, the FIN bit for the encoded frame will always be 1.
    # Frames are encoded as BINARY frames.
    def _encodeFrame(self, rawPayload, opCode, masked=1):
        ret = bytearray()
        # Op byte
        opByte = 0x80 | opCode  # Always a FIN, no RSV bits
        ret.append(opByte)
        # Payload Length bytes
        maskBit = masked
        payloadLength = len(rawPayload)
        if payloadLength <= 125:
            ret.append((maskBit << 7) | payloadLength)
        elif payloadLength <= 0xffff:  # 16-bit unsigned int
            ret.append((maskBit << 7) | 126)
            ret.extend(struct.pack("!H", payloadLength))
        elif payloadLength <= 0x7fffffffffffffff:  # 64-bit unsigned int (most significant bit must be 0)
            ret.append((maskBit << 7) | 127)
            ret.extend(struct.pack("!Q", payloadLength))
        else:  # Overflow
            raise ValueError("Exceeds the maximum number of bytes for a single websocket frame.")
        if maskBit == 1:
            # Mask key bytes
            maskKey = self._generateMaskKey()
            ret.extend(maskKey)
        # Mask the payload
        payloadBytes = bytearray(rawPayload)
        if maskBit == 1:
            for i in range(0, payloadLength):
                payloadBytes[i] ^= maskKey[i % 4]
        ret.extend(payloadBytes)
        # Return the assembled wss frame
        return ret

    # Used for the wss client to close a wss connection
    # Create and send a masked wss closing frame
    def _closeWssConnection(self):
        # Frames sent from client to server must be masked
        self._sslSocket.write(self._encodeFrame(b"", self._OP_CONNECTION_CLOSE, masked=1))

    # Used for the wss client to respond to a wss PING from server
    # Create and send a masked PONG frame
    def _sendPONG(self):
        # Frames sent from client to server must be masked
        self._sslSocket.write(self._encodeFrame(b"", self._OP_PONG, masked=1))

    # Override sslSocket read. Always read from the wss internal payload buffer, which
    # contains the masked MQTT packet. This read will decode ONE wss frame every time
    # and load in the payload for MQTT _packet_read. At any time, MQTT _packet_read
    # should be able to read a complete MQTT packet from the payload (buffered per wss
    # frame payload). If the MQTT packet is break into separate wss frames, different
    # chunks will be buffered in separate frames and MQTT _packet_read will not be able
    # to collect a complete MQTT packet to operate on until the necessary payload is
    # fully buffered.
    # If the requested number of bytes are not available, SSL_ERROR_WANT_READ will be
    # raised to trigger another call of _packet_read when the data is available again.
    def read(self, numberOfBytes):
        # Check if we have enough data for paho
        # _payloadDataBuffer will not be empty ony when the payload of a new wss frame
        # has been unmasked.
        if len(self._payloadDataBuffer) >= numberOfBytes:
            ret = self._payloadDataBuffer[0:numberOfBytes]
            self._payloadDataBuffer = self._payloadDataBuffer[numberOfBytes:]
            # struct.unpack(fmt, string) # Py2.x
            # struct.unpack(fmt, buffer) # Py3.x
            # Here ret is always in bytes (buffer interface)
            if sys.version_info[0] < 3:  # Py2.x
                ret = str(ret)
            return ret
        # Emmm, We don't. Try to buffer from the socket (It's a new wss frame).
        if not self._hasOpByte:  # Check if we need to buffer OpByte
            opByte = self._bufferedReader.read(1)
            self._isFIN = (opByte[0] & 0x80) == 0x80
            self._RSVBits = (opByte[0] & 0x70)
            self._opCode = (opByte[0] & 0x0f)
            self._hasOpByte = True  # Finished buffering opByte
            # Check if any of the RSV bits are set, if so, close the connection
            # since client never sends negotiated extensions
            if self._RSVBits != 0x0:
                self._closeWssConnection()
                self._connectStatus = self._WebsocketDisconnected
                self._payloadDataBuffer = bytearray()
                raise socket.error(ssl.SSL_ERROR_WANT_READ, "RSV bits set with NO negotiated extensions.")
        if not self._hasPayloadLengthFirst:  # Check if we need to buffer First Payload Length byte
            payloadLengthFirst = self._bufferedReader.read(1)
            self._hasPayloadLengthFirst = True  # Finished buffering first byte of payload length
            self._needMaskKey = (payloadLengthFirst[0] & 0x80) == 0x80
            payloadLengthFirstByteArray = bytearray()
            payloadLengthFirstByteArray.extend(payloadLengthFirst)
            self._payloadLength = (payloadLengthFirstByteArray[0] & 0x7f)

            if self._payloadLength == 126:
                self._payloadLengthBytesLength = 2
                self._hasPayloadLengthExtended = False  # Force to buffer the extended
            elif self._payloadLength == 127:
                self._payloadLengthBytesLength = 8
                self._hasPayloadLengthExtended = False  # Force to buffer the extended
            else:  # _payloadLength <= 125:
                self._hasPayloadLengthExtended = True  # No need to buffer extended payload length
        if not self._hasPayloadLengthExtended:  # Check if we need to buffer Extended Payload Length bytes
            payloadLengthExtended = self._bufferedReader.read(self._payloadLengthBytesLength)
            self._hasPayloadLengthExtended = True
            if sys.version_info[0] < 3:
                payloadLengthExtended = str(payloadLengthExtended)
            if self._payloadLengthBytesLength == 2:
                self._payloadLength = struct.unpack("!H", payloadLengthExtended)[0]
            else:  # _payloadLengthBytesLength == 8
                self._payloadLength = struct.unpack("!Q", payloadLengthExtended)[0]

        if self._needMaskKey:  # Response from server is masked, close the connection
            self._closeWssConnection()
            self._connectStatus = self._WebsocketDisconnected
            self._payloadDataBuffer = bytearray()
            raise socket.error(ssl.SSL_ERROR_WANT_READ, "Server response masked, closing connection and try again.")

        if not self._hasPayload:  # Check if we need to buffer the payload
            payloadForThisFrame = self._bufferedReader.read(self._payloadLength)
            self._hasPayload = True
            # Client side should never received a masked packet from the server side
            # Unmask it as needed
            #if self._needMaskKey:
            #    for i in range(0, self._payloadLength):
            #        payloadForThisFrame[i] ^= self._maskKey[i % 4]
            # Append it to the internal payload buffer
            self._payloadDataBuffer.extend(payloadForThisFrame)
        # Now we have the complete wss frame, reset the context
        # Check to see if it is a wss closing frame
        if self._opCode == self._OP_CONNECTION_CLOSE:
            self._connectStatus = self._WebsocketDisconnected
            self._payloadDataBuffer = bytearray()  # Ensure that once the wss closing frame comes, we have nothing to read and start all over again
        # Check to see if it is a wss PING frame
        if self._opCode == self._OP_PING:
            self._sendPONG()  # Nothing more to do here, if the transmission of the last wssMQTT packet is not finished, it will continue
        self._reset()
        # Check again if we have enough data for paho
        if len(self._payloadDataBuffer) >= numberOfBytes:
            ret = self._payloadDataBuffer[0:numberOfBytes]
            self._payloadDataBuffer = self._payloadDataBuffer[numberOfBytes:]
            # struct.unpack(fmt, string) # Py2.x
            # struct.unpack(fmt, buffer) # Py3.x
            # Here ret is always in bytes (buffer interface)
            if sys.version_info[0] < 3:  # Py2.x
                ret = str(ret)
            return ret
        else:  # Fragmented MQTT packets in separate wss frames
            raise socket.error(ssl.SSL_ERROR_WANT_READ, "Not a complete MQTT packet payload within this wss frame.")

    def write(self, bytesToBeSent):
        # When there is a disconnection, select will report a TypeError which triggers the reconnect.
        # In reconnect, Paho will set the socket object (mocked by wss) to None, blocking other ops
        # before a connection is re-established.
        # This 'low-level' socket write op should always be able to write to plain socket.
        # Error reporting is performed by Python socket itself.
        # Wss closing frame handling is performed in the wss read.
        return self._bufferedWriter.write(self._encodeFrame(bytesToBeSent, self._OP_BINARY, 1), len(bytesToBeSent))

    def close(self):
        if self._sslSocket is not None:
            self._sslSocket.close()
            self._sslSocket = None

    def getpeercert(self):
        return self._sslSocket.getpeercert()

    def getSSLSocket(self):
        if self._connectStatus != self._WebsocketDisconnected:
            return self._sslSocket
        else:
            return None  # Leave the sslSocket to Paho to close it. (_ssl.close() -> wssCore.close())
