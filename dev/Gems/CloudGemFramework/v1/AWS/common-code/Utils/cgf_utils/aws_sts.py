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
import boto3


class AWSSTSUtils:
    """Util class to help with construction of STS clients because boto3 doesn't automatically call
    the STS regional endpoints: https://github.com/boto/boto3/issues/1859

    AWS recommends using Regional AWS STS endpoints instead of the global endpoint to reduce latency, build in redundancy,
    and increase session token validity
    """
    STS_SERVICE_NAME = 'sts'

    def __init__(self, region):
        self._region = region
        self._endpoint_url = self.__get_sts_endpoint(region)

    @property
    def endpoint_url(self):
        """Get the STS regional endpoint to call"""
        return self._endpoint_url

    def client(self, session=None):
        """Creates a new STS client in the current session if provided"""
        if session is not None:
            return session.client(self.STS_SERVICE_NAME, endpoint_url=self._endpoint_url)
        else:
            return boto3.client(self.STS_SERVICE_NAME, endpoint_url=self._endpoint_url, region_name=self._region)

    def client_with_credentials(self, aws_access_key_id, aws_secret_access_key, aws_session_token=None):
        """Create a new STS client in a new session using credentials provided."""
        session = boto3.Session(
            aws_access_key_id=aws_access_key_id,
            aws_secret_access_key=aws_secret_access_key,
            aws_session_token=aws_session_token,
            region_name=self._region
        )
        return self.client(session=session)

    @staticmethod
    def __get_sts_endpoint(region):
        """
        Get the regional AWS STS endpoint for the provided region
        :param region: [Optiona;] The region to use, otherwise provide the global endpoint
        :return: The appropriate regional or global STS endpoint
        """
        if region is not None:
            return "https://sts.{region}.amazonaws.com".format(region=region)
        else:
            print("No region provided. Defaulting to global STS endpoint")
            return "https://sts.amazonaws.com"

    @staticmethod
    def validate_session_token(session_token):
        if str(session_token).startswith('F'):
            raise RuntimeError("ERROR: STS session token is from global endpoint: {token}".format(token=session_token))
