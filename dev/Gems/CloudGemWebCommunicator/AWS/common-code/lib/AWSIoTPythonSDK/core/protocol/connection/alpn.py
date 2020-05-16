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


try:
    import ssl
except:
    ssl = None


class SSLContextBuilder(object):

    def __init__(self):
        self.check_supportability()
        self._ssl_context = ssl.create_default_context()

    def check_supportability(self):
        if ssl is None:
            raise RuntimeError("This platform has no SSL/TLS.")
        if not hasattr(ssl, "SSLContext"):
            raise NotImplementedError("This platform does not support SSLContext. Python 2.7.10+/3.5+ is required.")
        if not hasattr(ssl.SSLContext, "set_alpn_protocols"):
            raise NotImplementedError("This platform does not support ALPN as TLS extensions. Python 2.7.10+/3.5+ is required.")

    def with_ca_certs(self, ca_certs):
        self._ssl_context.load_verify_locations(ca_certs)
        return self

    def with_cert_key_pair(self, cert_file, key_file):
        self._ssl_context.load_cert_chain(cert_file, key_file)
        return self

    def with_cert_reqs(self, cert_reqs):
        self._ssl_context.verify_mode = cert_reqs
        return self

    def with_check_hostname(self, check_hostname):
        self._ssl_context.check_hostname = check_hostname
        return self

    def with_ciphers(self, ciphers):
        if ciphers is not None:
            self._ssl_context.set_ciphers(ciphers)  # set_ciphers() does not allow None input. Use default (do nothing) if None
        return self

    def with_alpn_protocols(self, alpn_protocols):
        self._ssl_context.set_alpn_protocols(alpn_protocols)
        return self

    def build(self):
        return self._ssl_context
