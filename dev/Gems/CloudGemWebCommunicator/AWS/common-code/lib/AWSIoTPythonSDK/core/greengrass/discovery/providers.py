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


from AWSIoTPythonSDK.exception.AWSIoTExceptions import DiscoveryInvalidRequestException
from AWSIoTPythonSDK.exception.AWSIoTExceptions import DiscoveryUnauthorizedException
from AWSIoTPythonSDK.exception.AWSIoTExceptions import DiscoveryDataNotFoundException
from AWSIoTPythonSDK.exception.AWSIoTExceptions import DiscoveryThrottlingException
from AWSIoTPythonSDK.exception.AWSIoTExceptions import DiscoveryTimeoutException
from AWSIoTPythonSDK.exception.AWSIoTExceptions import DiscoveryFailure
from AWSIoTPythonSDK.core.greengrass.discovery.models import DiscoveryInfo
import re
import sys
import ssl
import time
import errno
import logging
import socket
import platform
if platform.system() == 'Windows':
    EAGAIN = errno.WSAEWOULDBLOCK
else:
    EAGAIN = errno.EAGAIN


class DiscoveryInfoProvider(object):

    REQUEST_TYPE_PREFIX = "GET "
    PAYLOAD_PREFIX = "/greengrass/discover/thing/"
    PAYLOAD_SUFFIX = " HTTP/1.1\r\n\r\n" # Space in the front
    HTTP_PROTOCOL = r"HTTP/1.1 "
    CONTENT_LENGTH = r"content-length: "
    CONTENT_LENGTH_PATTERN = CONTENT_LENGTH + r"([0-9]+)\r\n"
    HTTP_RESPONSE_CODE_PATTERN = HTTP_PROTOCOL + r"([0-9]+) "

    HTTP_SC_200 = "200"
    HTTP_SC_400 = "400"
    HTTP_SC_401 = "401"
    HTTP_SC_404 = "404"
    HTTP_SC_429 = "429"

    LOW_LEVEL_RC_COMPLETE = 0
    LOW_LEVEL_RC_TIMEOUT = -1

    _logger = logging.getLogger(__name__)

    def __init__(self, caPath="", certPath="", keyPath="", host="", port=8443, timeoutSec=120):
        """

        The class that provides functionality to perform a Greengrass discovery process to the cloud.

        Users can perform Greengrass discovery process for a specific Greengrass aware device to retrieve
        connectivity/identity information of Greengrass cores within the same group.

        **Syntax**

        .. code:: python

          from AWSIoTPythonSDK.core.greengrass.discovery.providers import DiscoveryInfoProvider

          # Create a discovery information provider
          myDiscoveryInfoProvider = DiscoveryInfoProvider()
          # Create a discovery information provider with custom configuration
          myDiscoveryInfoProvider = DiscoveryInfoProvider(caPath=myCAPath, certPath=myCertPath, keyPath=myKeyPath, host=myHost, timeoutSec=myTimeoutSec)

        **Parameters**

        *caPath* - Path to read the root CA file.

        *certPath* - Path to read the certificate file.

        *keyPath* - Path to read the private key file.

        *host* - String that denotes the host name of the user-specific AWS IoT endpoint.

        *port* - Integer that denotes the port number to connect to. For discovery purpose, it is 8443 by default.

        *timeoutSec* - Time out configuration in seconds to consider a discovery request sending/response waiting has
        been timed out.

        **Returns**

        AWSIoTPythonSDK.core.greengrass.discovery.providers.DiscoveryInfoProvider object

        """
        self._ca_path = caPath
        self._cert_path = certPath
        self._key_path = keyPath
        self._host = host
        self._port = port
        self._timeout_sec = timeoutSec
        self._expected_exception_map = {
            self.HTTP_SC_400 : DiscoveryInvalidRequestException(),
            self.HTTP_SC_401 : DiscoveryUnauthorizedException(),
            self.HTTP_SC_404 : DiscoveryDataNotFoundException(),
            self.HTTP_SC_429 : DiscoveryThrottlingException()
        }

    def configureEndpoint(self, host, port=8443):
        """

        **Description**

        Used to configure the host address and port number for the discovery request to hit. Should be called before
        the discovery request happens.

        **Syntax**

        .. code:: python

          # Using default port configuration, 8443
          myDiscoveryInfoProvider.configureEndpoint(host="prefix.iot.us-east-1.amazonaws.com")
          # Customize port configuration
          myDiscoveryInfoProvider.configureEndpoint(host="prefix.iot.us-east-1.amazonaws.com", port=8888)

        **Parameters**

        *host* - String that denotes the host name of the user-specific AWS IoT endpoint.

        *port* - Integer that denotes the port number to connect to. For discovery purpose, it is 8443 by default.

        **Returns**

        None

        """
        self._host = host
        self._port = port

    def configureCredentials(self, caPath, certPath, keyPath):
        """

        **Description**

        Used to configure the credentials for discovery request. Should be called before the discovery request happens.

        **Syntax**

        .. code:: python

          myDiscoveryInfoProvider.configureCredentials("my/ca/path", "my/cert/path", "my/key/path")

        **Parameters**

        *caPath* - Path to read the root CA file.

        *certPath* - Path to read the certificate file.

        *keyPath* - Path to read the private key file.

        **Returns**

        None

        """
        self._ca_path = caPath
        self._cert_path = certPath
        self._key_path = keyPath

    def configureTimeout(self, timeoutSec):
        """

        **Description**

        Used to configure the time out in seconds for discovery request sending/response waiting. Should be called before
        the discovery request happens.

        **Syntax**

        .. code:: python

          # Configure the time out for discovery to be 10 seconds
          myDiscoveryInfoProvider.configureTimeout(10)

        **Parameters**

        *timeoutSec* - Time out configuration in seconds to consider a discovery request sending/response waiting has
        been timed out.

        **Returns**

        None

        """
        self._timeout_sec = timeoutSec

    def discover(self, thingName):
        """

        **Description**
        
        Perform the discovery request for the given Greengrass aware device thing name.

        **Syntax**

        .. code:: python

          myDiscoveryInfoProvider.discover(thingName="myGGAD")

        **Parameters**

        *thingName* - Greengrass aware device thing name.

        **Returns**

        :code:`AWSIoTPythonSDK.core.greengrass.discovery.models.DiscoveryInfo` object.

        """
        self._logger.info("Starting discover request...")
        self._logger.info("Endpoint: " + self._host + ":" + str(self._port))
        self._logger.info("Target thing: " + thingName)
        sock = self._create_tcp_connection()
        ssl_sock = self._create_ssl_connection(sock)
        self._raise_on_timeout(self._send_discovery_request(ssl_sock, thingName))
        status_code, response_body = self._receive_discovery_response(ssl_sock)

        return self._raise_if_not_200(status_code, response_body)

    def _create_tcp_connection(self):
        self._logger.debug("Creating tcp connection...")
        try:
            if (sys.version_info[0] == 2 and sys.version_info[1] < 7) or (sys.version_info[0] == 3 and sys.version_info[1] < 2):
                sock = socket.create_connection((self._host, self._port))
            else:
                sock = socket.create_connection((self._host, self._port), source_address=("", 0))
            return sock
        except socket.error as err:
            if err.errno != errno.EINPROGRESS and err.errno != errno.EWOULDBLOCK and err.errno != EAGAIN:
                raise
        self._logger.debug("Created tcp connection.")

    def _create_ssl_connection(self, sock):
        self._logger.debug("Creating ssl connection...")
        ssl_sock = ssl.wrap_socket(sock,
                                   certfile=self._cert_path,
                                   keyfile=self._key_path,
                                   ca_certs=self._ca_path,
                                   cert_reqs=ssl.CERT_REQUIRED,
                                   ssl_version=ssl.PROTOCOL_SSLv23)
        self._logger.debug("Matching host name...")
        if sys.version_info[0] < 3 or (sys.version_info[0] == 3 and sys.version_info[1] < 2):
            self._tls_match_hostname(ssl_sock)
        else:
            ssl.match_hostname(ssl_sock.getpeercert(), self._host)

        return ssl_sock

    def _tls_match_hostname(self, ssl_sock):
        try:
            cert = ssl_sock.getpeercert()
        except AttributeError:
            # the getpeercert can throw Attribute error: object has no attribute 'peer_certificate'
            # Don't let that crash the whole client. See also: http://bugs.python.org/issue13721
            raise ssl.SSLError('Not connected')

        san = cert.get('subjectAltName')
        if san:
            have_san_dns = False
            for (key, value) in san:
                if key == 'DNS':
                    have_san_dns = True
                    if self._host_matches_cert(self._host.lower(), value.lower()) == True:
                        return
                if key == 'IP Address':
                    have_san_dns = True
                    if value.lower() == self._host.lower():
                        return

            if have_san_dns:
                # Only check subject if subjectAltName dns not found.
                raise ssl.SSLError('Certificate subject does not match remote hostname.')
        subject = cert.get('subject')
        if subject:
            for ((key, value),) in subject:
                if key == 'commonName':
                    if self._host_matches_cert(self._host.lower(), value.lower()) == True:
                        return

        raise ssl.SSLError('Certificate subject does not match remote hostname.')

    def _host_matches_cert(self, host, cert_host):
        if cert_host[0:2] == "*.":
            if cert_host.count("*") != 1:
                return False

            host_match = host.split(".", 1)[1]
            cert_match = cert_host.split(".", 1)[1]
            if host_match == cert_match:
                return True
            else:
                return False
        else:
            if host == cert_host:
                return True
            else:
                return False

    def _send_discovery_request(self, ssl_sock, thing_name):
        request = self.REQUEST_TYPE_PREFIX + \
                  self.PAYLOAD_PREFIX + \
                  thing_name + \
                  self.PAYLOAD_SUFFIX
        self._logger.debug("Sending discover request: " + request)

        start_time = time.time()
        desired_length_to_write = len(request)
        actual_length_written = 0
        while True:
            try:
                length_written = ssl_sock.write(request.encode("utf-8"))
                actual_length_written += length_written
            except socket.error as err:
                if err.errno == ssl.SSL_ERROR_WANT_READ or err.errno == ssl.SSL_ERROR_WANT_WRITE:
                    pass
            if actual_length_written == desired_length_to_write:
                return self.LOW_LEVEL_RC_COMPLETE
            if start_time + self._timeout_sec < time.time():
                return self.LOW_LEVEL_RC_TIMEOUT

    def _receive_discovery_response(self, ssl_sock):
        self._logger.debug("Receiving discover response header...")
        rc1, response_header = self._receive_until(ssl_sock, self._got_two_crlfs)
        status_code, body_length = self._handle_discovery_response_header(rc1, response_header.decode("utf-8"))

        self._logger.debug("Receiving discover response body...")
        rc2, response_body = self._receive_until(ssl_sock, self._got_enough_bytes, body_length)
        response_body = self._handle_discovery_response_body(rc2, response_body.decode("utf-8"))

        return status_code, response_body

    def _receive_until(self, ssl_sock, criteria_function, extra_data=None):
        start_time = time.time()
        response = bytearray()
        number_bytes_read = 0
        while True:  # Python does not have do-while
            try:
                response.append(self._convert_to_int_py3(ssl_sock.read(1)))
                number_bytes_read += 1
            except socket.error as err:
                if err.errno == ssl.SSL_ERROR_WANT_READ or err.errno == ssl.SSL_ERROR_WANT_WRITE:
                    pass

            if criteria_function((number_bytes_read, response, extra_data)):
                return self.LOW_LEVEL_RC_COMPLETE, response
            if start_time + self._timeout_sec < time.time():
                return self.LOW_LEVEL_RC_TIMEOUT, response

    def _convert_to_int_py3(self, input_char):
        try:
            return ord(input_char)
        except:
            return input_char

    def _got_enough_bytes(self, data):
        number_bytes_read, response, target_length = data
        return number_bytes_read == int(target_length)

    def _got_two_crlfs(self, data):
        number_bytes_read, response, extra_data_unused = data
        number_of_crlf = 2
        has_enough_bytes = number_bytes_read > number_of_crlf * 2 - 1
        if has_enough_bytes:
            end_of_received = response[number_bytes_read - number_of_crlf * 2 : number_bytes_read]
            expected_end_of_response = b"\r\n" * number_of_crlf
            return end_of_received == expected_end_of_response
        else:
            return False

    def _handle_discovery_response_header(self, rc, response):
        self._raise_on_timeout(rc)
        http_status_code_matcher = re.compile(self.HTTP_RESPONSE_CODE_PATTERN)
        http_status_code_matched_groups = http_status_code_matcher.match(response)
        content_length_matcher = re.compile(self.CONTENT_LENGTH_PATTERN)
        content_length_matched_groups = content_length_matcher.search(response)
        return http_status_code_matched_groups.group(1), content_length_matched_groups.group(1)

    def _handle_discovery_response_body(self, rc, response):
        self._raise_on_timeout(rc)
        return response

    def _raise_on_timeout(self, rc):
        if rc == self.LOW_LEVEL_RC_TIMEOUT:
            raise DiscoveryTimeoutException()

    def _raise_if_not_200(self, status_code, response_body):  # response_body here is str in Py3
        if status_code != self.HTTP_SC_200:
            expected_exception = self._expected_exception_map.get(status_code)
            if expected_exception:
                raise expected_exception
            else:
                raise DiscoveryFailure(response_body)
        return DiscoveryInfo(response_body)
