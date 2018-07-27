import resource_manager_common.constant as constant
import boto3
from botocore.exceptions import ClientError
from resource_manager.test import lmbr_aws_test_support
from requests_aws4auth import AWS4Auth
import requests
import re
import sys
sys.path.append('./Gems/CloudGemMetric/v1/AWS/common-code/Constant')
sys.path.append('./Gems/CloudGemMetric/v1/AWS/common-code/AWSCommon')
import metric_constant as c

class IntegrationTest_CloudGemMetric(lmbr_aws_test_support.lmbr_aws_TestCase):

    GEM_NAME = 'CloudGemMetric'
    TEST_DEPLOYMENT_NAME_1 = "d1"

    def __init__(self, *args, **kwargs):
        super(IntegrationTest_CloudGemMetric, self).__init__(*args, **kwargs)

    def setUp(self):
        self.prepare_test_envionment("cloud_gem_metric")

    def test_end_to_end(self):
        self.run_all_tests()

    ''' TESTS '''
    def __000_create_stacks(self):
        self.enable_real_gem(self.GEM_NAME, "v1")

        if not self.__has_project_stack():
            self.lmbr_aws('project', 'create', '--stack-name', self.TEST_PROJECT_STACK_NAME, '--confirm-aws-usage', '--confirm-security-change', '--region', lmbr_aws_test_support.REGION)
        else:
            print 'Reusing existing project stack {}'.format(self.get_project_stack_arn())

        if not self.__has_deployment_stack():
            self.lmbr_aws('deployment', 'create', '--deployment', self.TEST_DEPLOYMENT_NAME_1, '--confirm-aws-usage', '--confirm-security-change')        

        
    def __001_stop_scheduled_lamdas(self):
        deployment_stack_arn = self.get_deployment_stack_arn(self.TEST_DEPLOYMENT_NAME_1)
        resource_stack_arn =  self.get_stack_resource_arn(deployment_stack_arn, c.RES_GEM_NAME)
        amoeba = self.get_stack_resource_physical_id(resource_stack_arn, c.ENV_AMOEBA_LAUNCHER_RULE)
        consumer = self.get_stack_resource_physical_id(resource_stack_arn, c.ENV_LAMBDA_LAUNCHER_RULE)
        
        self.assertIsNotNone(amoeba)
        self.assertIsNotNone(consumer)        
        client = boto3.client('events', region_name=lmbr_aws_test_support.REGION)        

        print client.disable_rule(
                        Name=amoeba
                    )
        print client.disable_rule(
                        Name=consumer
                    )        

    def __010_create_metric_use_lambda_no_chunking(self):
        self.lmbr_aws('metric', 'send-test-metrics', '--threads', '1', '--iterations-per-thread', '1', '--events-per-iteration', '5', '--use-lambda')
        self.assertIn("test data with 5 events.", self.lmbr_aws_stdout)
        self.assertIn("Invoking lambda", self.lmbr_aws_stdout) 

        self.lmbr_aws('metric', 'send-test-metrics', '--use-lambda')
        self.assertIn("test data with 1 events.", self.lmbr_aws_stdout)
        self.assertIn("Invoking lambda", self.lmbr_aws_stdout) 

        self.lmbr_aws('metric', 'send-test-metrics', '--threads', '5', '--iterations-per-thread', '10', '--events-per-iteration', '2', '--use-lambda')        
        self.assertIn("test data with 2 events.", self.lmbr_aws_stdout)
        self.assertIn("metrics have been sent to the FIFO queue", self.lmbr_aws_stdout) 
        
        self.lmbr_aws('metric', 'consume')           
        self.assertIn("[None]Saving aggregated metrics to S3 bucket 'cctest", self.lmbr_aws_stdout)        
        self.assertIn("[None]Deleting messages from SQS queue 'https://queue.amazonaws.com/", self.lmbr_aws_stdout)        

    def __020_create_metric_use_lambda_chunking(self):

        self.lmbr_aws('metric', 'send-test-metrics', '--iterations-per-thread', '1', '--events-per-iteration', '2000')        

        self.assertIn("[None]Using SQS queue URL 'https://queue.amazonaws.com/", self.lmbr_aws_stdout)
        self.assertIn("2000 metrics have been sent to the FIFO queue", self.lmbr_aws_stdout)
        self.assertTrue(len(re.findall(r'Sending a sqs message with', self.lmbr_aws_stdout, re.IGNORECASE)) >= 2, "There was not at least 2 message chunks")                
        
        self.lmbr_aws('metric', 'consume')           
        self.assertIn("[None]Saving aggregated metrics to S3 bucket 'cctest", self.lmbr_aws_stdout)        
        self.assertIn("[None]Deleting messages from SQS queue 'https://queue.amazonaws.com/", self.lmbr_aws_stdout)        


    def __040_generate_amoeba_file(self):        
        deployment_stack_arn = self.get_deployment_stack_arn(self.TEST_DEPLOYMENT_NAME_1)
        resource_stack_arn =  self.get_stack_resource_arn(deployment_stack_arn, c.RES_GEM_NAME)
        bucket = self.get_stack_resource_physical_id(resource_stack_arn, c.ENV_S3_STORAGE)
        self.lmbr_aws('metric', 'send-test-metrics', '--events-per-iteration', '10', '--iterations-per-thread', '20', '--threads', '16')
        self.lmbr_aws('metric', 'consume')   
        self.lmbr_aws('metric', 'send-test-metrics', '--events-per-iteration', '10', '--iterations-per-thread', '20', '--threads', '16')
        self.lmbr_aws('metric', 'consume')   
        self.lmbr_aws('metric', 'combine-s3-files', '--detailed-logs')        
        
        self.assertIn("Running crawler on bucket '{}'".format(bucket), self.lmbr_aws_stdout)
        self.assertIn("Saving the ingested file to '", self.lmbr_aws_stdout)        
        self.assertIn("Launched ", self.lmbr_aws_stdout)          

    def __050_run_the_glue_crawler(self):        
        self.lmbr_aws('metric', 'glue', '--function', 'start_crawler')        

        self.assertIn("RUNNING", self.lmbr_aws_stdout)
        self.assertIn("STOPPING", self.lmbr_aws_stdout)
        self.assertIn("READY", self.lmbr_aws_stdout)

    def __060_validating_metrics_with_athena(self):
        prefix = "{}_{}".format( self.TEST_PROJECT_STACK_NAME, self.TEST_DEPLOYMENT_NAME_1 )
        
        self.lmbr_aws('metric', 'query', '--sql', 'SELECT source FROM {0}.{0}_{1}_insensitive LIMIT 10'.format(prefix, c.RES_GEM_NAME.lower()))        
        self.assertIn("Query is RUNNING", self.lmbr_aws_stdout)
        self.assertIn("Query is SUCCEEDED", self.lmbr_aws_stdout)
        self.assertIn("['{0}'], ['{0}']".format(c.RES_GEM_NAME.lower()), self.lmbr_aws_stdout)

        self.lmbr_aws('metric', 'query', '--sql', 'SELECT source FROM {0}.{0}_{1}_sensitive LIMIT 10'.format(prefix, c.RES_GEM_NAME.lower()))        
        self.assertIn("Query is RUNNING", self.lmbr_aws_stdout)
        self.assertIn("Query is SUCCEEDED", self.lmbr_aws_stdout)
        self.assertIn("['{0}'], ['{0}']".format(c.RES_GEM_NAME.lower()), self.lmbr_aws_stdout)

    def __has_project_stack(self):
        return bool(self.get_project_stack_arn())

    def __has_deployment_stack(self):
        return bool(self.get_deployment_stack_arn(self.TEST_DEPLOYMENT_NAME))