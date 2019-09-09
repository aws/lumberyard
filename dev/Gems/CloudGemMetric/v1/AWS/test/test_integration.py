import resource_manager_common.constant as constant
import boto3
from botocore.exceptions import ClientError
from resource_manager.test import lmbr_aws_test_support
from requests_aws4auth import AWS4Auth
from resource_manager.test import base_stack_test
import requests
import re
import sys
import time
sys.path.append('./Gems/CloudGemMetric/v1/AWS/common-code/Constant')
sys.path.append('./Gems/CloudGemMetric/v1/AWS/common-code/AWSCommon')
import metric_constant as c

class IntegrationTest_CloudGemMetric(base_stack_test.BaseStackTestCase):

    GEM_NAME = 'CloudGemMetric'

    def __init__(self, *args, **kwargs):
        super(IntegrationTest_CloudGemMetric, self).__init__(*args, **kwargs)

    def setUp(self):
        self.prepare_test_environment("cloud_gem_metric")
        self.register_for_shared_resources()
        self.enable_shared_gem(self.GEM_NAME, "v1")

    def test_end_to_end(self):
        self.run_all_tests()

    ''' TESTS '''
    def __000_create_stacks(self):
        self.setup_base_stack()
        self.lmbr_aws('deployment', 'default', '--set', self.TEST_DEPLOYMENT_NAME, '--project')
        self.lmbr_aws('deployment', 'default', '--set', self.TEST_DEPLOYMENT_NAME)
        
    def __001_stop_scheduled_lamdas(self):
        deployment_stack_arn = self.get_deployment_stack_arn(self.TEST_DEPLOYMENT_NAME)
        resource_stack_arn =  self.get_stack_resource_arn(deployment_stack_arn, c.RES_GEM_NAME)
        amoeba_resource = self.get_stack_resource_physical_id(resource_stack_arn, c.ENV_AMOEBA_LAUNCHER_RULE)        
        consumer_resource = self.get_stack_resource_physical_id(resource_stack_arn, c.ENV_LAMBDA_LAUNCHER_RULE)
        amoeba = self.get_stack_resource_physical_id(resource_stack_arn, c.ENV_AMOEBA_LAUNCHER)
        consumer = self.get_stack_resource_physical_id(resource_stack_arn, c.ENV_LAMBDA_CONSUMER_LAUNCHER)

        self.assertIsNotNone(amoeba_resource)
        self.assertIsNotNone(consumer_resource)        
        events_client = boto3.client('events', region_name=lmbr_aws_test_support.REGION)                
        
        print "Deleting rule {} for lambda {}".format(amoeba_resource, amoeba), events_client.remove_targets(
                                                Rule=amoeba_resource,
                                                Ids=[
                                                    amoeba,
                                                ]
                                            ) 
        print "Deleting rule {} for lambda {}".format(consumer_resource, consumer), events_client.remove_targets(
                                                Rule=consumer_resource,
                                                Ids=[
                                                    consumer,
                                                ]
                                            )         
        print events_client.put_rule(
            Name=consumer_resource,
            ScheduleExpression="rate(1 day)",
            State='DISABLED'
        )
        print events_client.put_rule(
            Name=amoeba_resource,
            ScheduleExpression="rate(1 day)",
            State='DISABLED'
        )        

    def __010_create_metric_use_lambda_no_chunking(self):
        self.lmbr_aws('metric', 'send-test-metrics', '--threads', '1', '--iterations-per-thread', '1', '--events-per-iteration', '5', '--use-lambda', '--event-type', 'bug')
        self.assertIn("test data with 5 events.", self.lmbr_aws_stdout)
        self.assertIn("StatusCode: 200", self.lmbr_aws_stdout)

        self.lmbr_aws('metric', 'send-test-metrics', '--use-lambda')
        self.assertIn("test data with 1 events.", self.lmbr_aws_stdout)
        self.assertIn("StatusCode: 200", self.lmbr_aws_stdout)

        self.lmbr_aws('metric', 'send-test-metrics', '--threads', '5', '--iterations-per-thread', '10', '--events-per-iteration', '2', '--use-lambda')        
        self.assertIn("test data with 2 events.", self.lmbr_aws_stdout)
        self.assertIn("metrics have been sent to the FIFO queue", self.lmbr_aws_stdout) 
        
        self.lmbr_aws('metric', 'consume')           
        self.assertIn("[None]Saving aggregated metrics to S3 bucket '{}".format(lmbr_aws_test_support.TEST_PREFIX), self.lmbr_aws_stdout)
        self.assertIn("[None]Deleting messages from SQS queue 'https://queue.amazonaws.com/", self.lmbr_aws_stdout)        

    def __020_create_metric_use_lambda_chunking(self):        
        self.lmbr_aws('metric', 'send-test-metrics', '--iterations-per-thread', '1', '--events-per-iteration', '3000', '--compression-type', 'nocompression', '--verbose')        

        self.assertIn("[None]Using SQS queue URL 'https://queue.amazonaws.com/", self.lmbr_aws_stdout)
        self.assertIn("3000 metrics have been sent to the FIFO queue", self.lmbr_aws_stdout)
        self.assertTrue(len(re.findall(r'Sending a sqs message with', self.lmbr_aws_stdout, re.IGNORECASE)) >= 2, "There was not at least 2 message chunks")                
        
        self.lmbr_aws('metric', 'consume', '--verbose')           
        self.assertIn("[None]Saving aggregated metrics to S3 bucket '{}".format(lmbr_aws_test_support.TEST_PREFIX), self.lmbr_aws_stdout)
        self.assertIn("Save complete to path", self.lmbr_aws_stdout)
        self.assertIn("[None]Deleting messages from SQS queue 'https://queue.amazonaws.com/", self.lmbr_aws_stdout)        


    def __040_generate_amoeba_file(self):        
        deployment_stack_arn = self.get_deployment_stack_arn(self.TEST_DEPLOYMENT_NAME)
        resource_stack_arn =  self.get_stack_resource_arn(deployment_stack_arn, c.RES_GEM_NAME)
        bucket = self.get_stack_resource_physical_id(resource_stack_arn, c.ENV_S3_STORAGE)
        #To ensure we have multiple parquet files to combine 
        self.lmbr_aws('metric', 'send-test-metrics', '--events-per-iteration', '10', '--iterations-per-thread', '20', '--threads', '16', '--event-type', 'bug')
        self.lmbr_aws('metric', 'consume')   
        #No this is not by accident.  We call it a second time to ensure the amoeba generator has two files to work with.
        self.lmbr_aws('metric', 'send-test-metrics', '--events-per-iteration', '10', '--iterations-per-thread', '20', '--threads', '16', '--event-type', 'bug')
        self.lmbr_aws('metric', 'consume')           
        
        #The amoeba only processes files that are older than five minutes.
        time.sleep(320)    
        self.lmbr_aws('metric', 'combine-s3-files', '--verbose')
        self.assertIn("Ingesting the files", self.lmbr_aws_stdout)
        self.assertIn("Getting main file for", self.lmbr_aws_stdout)
        self.assertIn("Processing file", self.lmbr_aws_stdout)
        self.assertIn("Size on S3:", self.lmbr_aws_stdout)
        self.assertIn("Time remaining:", self.lmbr_aws_stdout)                
        self.assertIn("Saving ingested file to", self.lmbr_aws_stdout)         
        self.assertIn("I've consumed everything I can in bucket ", self.lmbr_aws_stdout)


    def __050_run_the_glue_crawler(self):        
        self.lmbr_aws('metric', 'glue-crawler')
        self.assertIn("Starting crawler", self.lmbr_aws_stdout)

        self.lmbr_aws('metric', 'glue-crawler')
        self.assertIn("'RUNNING'", self.lmbr_aws_stdout)
        #Allow the crawler time to complete the crawling before querying results
        time.sleep(300)        

    def __060_validating_metrics_with_athena(self):
        prefix = "{}_{}".format( self.TEST_PROJECT_STACK_NAME.replace('-','_'), self.TEST_DEPLOYMENT_NAME.replace('-','_') )
        
        self.lmbr_aws('metric', 'query', '--sql', 'SELECT p_event_name, p_server_timestamp_strftime, p_server_timestamp_year, p_server_timestamp_month, p_server_timestamp_day, p_server_timestamp_hour, p_event_source, p_client_build_identifier, p_data_sensitivity, p_event_schema_hash  FROM {0}.{0}_table_dummy_bug LIMIT 10'.format(prefix))        
        self.assertIn("Query 'SELECT p_event_name, p_server_...' is RUNNING", self.lmbr_aws_stdout)
        self.assertIn("Query 'SELECT p_event_name, p_server_...' is SUCCEEDED", self.lmbr_aws_stdout)

        self.assertIn("'dummy_bug',", self.lmbr_aws_stdout)        
        self.assertIn(", '{0}',".format(c.RES_GEM_NAME.lower()), self.lmbr_aws_stdout)

    def __999_teardown_base_stack(self):
        self.teardown_base_stack()
