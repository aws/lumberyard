import os
import json
import sys
import unittest

import boto3

from botocore.exceptions import ClientError

from resource_manager.test import lmbr_aws_test_support
from resource_manager.test import base_stack_test



class IntegrationTest_AWSLambdaLanguageSupport_EndToEnd(base_stack_test.BaseStackTestCase):
    gem_name = "AWSLambdaLanguageDemo"
    default_payload = b"""{ "data": 123123123  }"""
    dotnet_payload = b"""  "data": 123123123  """ # the default template for the dotnet lambda expects the starting and trailing brackets to be removed

    def setUp(self):
        self.prepare_test_environment("lambda_language_support")
        self.register_for_shared_resources()
        self.enable_shared_gem(self.gem_name, 'v1')
        self.client = self.session.client('lambda')

    def test_end_to_end(self):
        self.run_all_tests()

    def __000_setup_base_stack(self):
        self.setup_base_stack()
    
    def __001_call_java_lambda(self):
        function_id = self._get_function_id('JavaJarLambda')
        response_body = self._invoke_function(function_id, self.default_payload)
        self.assertEquals(response_body, '"hello world"')

    def __002_call_python_lambda(self):
        function_id = self._get_function_id('PythonLambda')
        response_body = self._invoke_function(
            function_id, self.default_payload)
        self.assertEquals(response_body, '"hello world"')

    def __003_call_dotnet_lambda(self):
        function_id = self._get_function_id('DotnetLambda')
        response_body = self._invoke_function(
            function_id, self.dotnet_payload)
        self.assertEquals(response_body, '"hello world"')

    def __004_call_golang_lambda(self):
        function_id = self._get_function_id('GoLambda')
        response_body = self._invoke_function(
            function_id, self.default_payload)
        self.assertEquals(response_body, '{"message":"Hello World","ok":true}')

    def __999_teardown_base_stack(self):
        self.teardown_base_stack()

    def _invoke_function(self, function_id, payload):
        response = self.client.invoke(FunctionName=function_id, Payload=payload)
        body = response['Payload'].read()
        return body

    def _get_function_id(self, logical_name):
        deployment_stack_arn = self.get_deployment_stack_arn(
            self.TEST_DEPLOYMENT_NAME)

        resource_stack_arn = self.get_stack_resource_arn(
                deployment_stack_arn, self.gem_name)

        function = self.get_stack_resource_physical_id(
            resource_stack_arn, logical_name)

        return function
