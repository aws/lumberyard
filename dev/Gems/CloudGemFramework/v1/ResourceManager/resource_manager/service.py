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
# Helper methods to interact with ServiceAPI endpoints
# There is no Swagger->Python client but you can construct calls via the cgf_service_client.Path
import datetime
import time
import typing

import boto3

import cgf_service_client
from .context import Context
from .errors import HandledError


class BaseService:
    """
    Base class to aid working with a ServiceAPI
    """

    def __init__(self, context: Context, stack_id: str):
        self._context = context
        self._stack_id = stack_id
        self._url = None

    def _get_stack_description(self) -> dict:
        return self._context.stack.describe_stack(stack_id=self._stack_id, optional=True)

    def _get_stack_output(self, resource_name) -> typing.Optional[str]:
        """
        Gets the list of outputs in given stack
        """
        description = self._get_stack_description()
        outputs = description.get('Outputs', [])
        for output in outputs:
            if output.get('OutputKey') == resource_name:
                return output.get('OutputValue')
        return None

    def _get_stack_resource(self, resource_name) -> str:
        """
        Gets the list of outputs in given stack
        """
        return self._context.stack.get_physical_resource_id(self._stack_id, resource_name)


class Service(BaseService):
    """
        Aids construction of a client to a CloudCanvas ServiceAPI
    """

    def __init__(self, context: Context, stack_id: str):
        super().__init__(context, stack_id)

    @property
    def url(self) -> str:
        """Gets the service API url for a stack's service endpoint."""
        if self._url is None:
            self._url = self._get_stack_output('ServiceUrl')
            if self._url is None:
                raise RuntimeError("Missing ServiceUrl stack output.")
        return self._url

    def get_service_client(self, session: boto3.Session = None, verbose: bool = False) -> cgf_service_client.Path:
        """
        Gets a service client for a stack's service api.
        Will use repack to pack any non dict responses into generic {'Results': response} 

        Arguments:
            session: A pre-made boto3 session, otherwise a new session is created
            verbose: If true, sets the client to be verbose, otherwise only error statements will be logged
        Returns a cgf_service_client.Path object that can be used to make requests.
        """
        if self._context.config.project_stack_id is None:
            raise HandledError("Project stack does not exist.")

        return cgf_service_client.for_url(self.url, session=session, verbose=verbose, repack=True)


class ServiceLogs(BaseService):
    """
        Provides log interface for a ServiceAPI Lambda's Cloudwatch log groups
        Note: There is a propagation delay in making logs available for insight.
    """
    LOG_LINE_LIMIT = 50  # Cloudwatch logs can only return 1MB of data, so limit to 50 lines
    MAX_WAIT_TIME_S = 20  # Max amount of time, in Seconds, to spend waiting for logs
    WAIT_PERIOD_S = 1  # Time between query results check, in Seconds

    # Cloudwatch insight Query to run, if this is changed, then updated _insight_formatter as well
    CW_INSIGHT_QUERY = f"fields @timestamp, @message | sort @timestamp desc | limit {LOG_LINE_LIMIT}"

    def __init__(self, context: Context, stack_id: str):
        super().__init__(context, stack_id)
        self._cw_client = None
        self._period = 10  # Default: look back 10 mins

    @property
    def period(self):
        return self._period

    @period.setter
    def period(self, period):
        self._period = int(period)

    def _insight_formatter(self, response: {}) -> [str]:
        """
        Parse a get_query_results response {} to reformat results

        Note: If CW_INSIGHT_QUERY is updated then this function needs updating
        """
        logs = []
        for results in response['results']:
            message = ""
            timestamp = ""

            for result in results:
                if result['field'] == "@timestamp":
                    timestamp = result['value']
                elif result['field'] == "@message":
                    message = result['value']
            logs.append(f'{timestamp}: {message}')
        return logs

    def get_logs(self, session: boto3.Session = None) -> [str]:
        if session is None:
            session = boto3.Session()

        self._cw_client = session.client('logs')

        # Find the service lambda and log group
        lambda_name = self._get_stack_resource(resource_name='ServiceLambda')
        log_group_name = f'/aws/lambda/{lambda_name}'

        # Query its log group via cw insight
        start_query_response = self._cw_client.start_query(
            logGroupName=log_group_name,
            startTime=int((datetime.datetime.today() - datetime.timedelta(minutes=self.period)).timestamp()),
            endTime=int(datetime.datetime.now().timestamp()),
            queryString=ServiceLogs.CW_INSIGHT_QUERY,
        )

        query_id = start_query_response['queryId']

        response = None

        elapsed = 0
        while response is None or response['status'] == 'Running':
            print('Waiting for query to complete ...')
            time.sleep(ServiceLogs.WAIT_PERIOD_S)
            response = self._cw_client.get_query_results(
                queryId=query_id
            )
            elapsed += ServiceLogs.WAIT_PERIOD_S
            if elapsed > ServiceLogs.MAX_WAIT_TIME_S:
                raise RuntimeError(f"Timed out retrieving service logs. Exceeded {ServiceLogs.MAX_WAIT_TIME_S} seconds.")

        logs = self._insight_formatter(response)
        return logs
