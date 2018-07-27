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
# $Revision: #1 $
import sys
import types
import resource_manager.cli

def add_cli_commands(hook, subparsers, add_common_args, **kwargs):
    subparser = subparsers.add_parser("metric", help="Commands to manage the Cloud Gem Metric gem")
    subparser.register('action', 'parsers', resource_manager.cli.AliasedSubParsersAction)
    metric_subparsers = subparser.add_subparsers(dest = 'subparser_name', metavar='command')

    #altering the path so we can execute lambda code locally
    add_modules()

    import lambda_launcher
    subparser = metric_subparsers.add_parser('consume', help='Run the metric fifo consumer locally')
    add_common_args(subparser)
    subparser.set_defaults(func=lambda_launcher.cli)
    
    import producer
    subparser = metric_subparsers.add_parser('send-test-metrics', help='Simulate producing metrics')
    subparser.add_argument('--sensitivity-type', required=False, default=None, help='Use S3 native AES256 encryption for the events generated. Options: sensitive | insensitive ')    
    subparser.add_argument('--threads', type=int, required=False, default=1, help='Number of threads to send metrics with.')    
    subparser.add_argument('--iterations-per-thread', type=int, required=False, default=1, help='Number of iterations per thread')    
    subparser.add_argument('--events-per-iteration', type=int, required=False, default=1, help='Number of events to send per iteration per thread')    
    subparser.add_argument('--erroneous-metrics', action='store_true', required=False, default=False, help='Send erroneous events')    
    subparser.add_argument('--sleep-duration-between-jobs', type=float, required=False, default=0, help='The period in seconds in which to sleep between thread job assignments')    
    subparser.add_argument('--use-lambda', action='store_true', required=False, default=False, help='Run the SQS message producer locally or invoke the producer lambda')    
    subparser.add_argument('--event-type', required=False, default='random_event', help='Use only a specific type of event.  Options: random_event | defect | shotfired | shotmissed | shotdodged | bug')    
    add_common_args(subparser)
    subparser.set_defaults(func=producer.cli)
    
    import amoeba_launcher
    subparser = metric_subparsers.add_parser('combine-s3-files', help='Combine S3 files into a single S3 file.')
    add_common_args(subparser)
    subparser.set_defaults(func=amoeba_launcher.cli)

    import graph
    subparser = metric_subparsers.add_parser('graph', help='View Cloudwatch metrics.')
    subparser.add_argument('--function', required=True, default='avg_save_duration', help='The graph api function to execute.  Options are: consumer_save_duration, consumer_messages_processed, consumer_bytes_processed, consumer_rows_added, lambda_invocations, consumer_sqs_delete_duration, dynamodb_context_read_provisioned, dynamodb_context_read_consumed, dynamodb_context_write_provisioned, dynamodb_context_write_consumed, avg_save_duration, avg_delete_duration')    
    add_common_args(subparser)
    subparser.set_defaults(func=graph.cli)

    import context
    subparser = metric_subparsers.add_parser('context', help='Run the context service APIs.')    
    subparser.add_argument('--function', required=True, default='get', help='The context api function to execute.  Options are: get, update_context, get_predefined_partition, get_partition, update_partition, get_filter, update_filter, get_priority, update_priority')    
    add_common_args(subparser)
    subparser.set_defaults(func=context.cli)

    import parquet_reader
    subparser = metric_subparsers.add_parser('open', help='Open a local .parquet type file.')
    subparser.add_argument('--file-path', required=False, default=None, help='The local file absolute path')        
    subparser.add_argument('--s3-key', required=False, default=None, help='The S3 key to open.  Example: /insensitive/cloudgemmetric')    
    add_common_args(subparser)
    subparser.set_defaults(func=parquet_reader.debug_file)

    import query_athena
    subparser = metric_subparsers.add_parser('query', help='Query Athena')
    subparser.add_argument('--sql', required=True, help='Execute a SQL query against the AWS Athena engine.')    
    add_common_args(subparser)
    subparser.set_defaults(func=query_athena.cli)

    import Custom_Glue
    subparser = metric_subparsers.add_parser('glue-setup', help='Execute a glue setup command')
    subparser.add_argument('--function', required=False, default='handler', help='The context api function to execute.  Options are: handle, start_crawler, stop_crawler')        
    subparser.add_argument('--event-type', required=True, default='UPDATE', help='UPDATE | CREATE | DELETE')        
    add_common_args(subparser)
    subparser.set_defaults(func=Custom_Glue.cli)

    import glue_crawler
    subparser = metric_subparsers.add_parser('glue-crawler', help='Execute a glue command')    
    add_common_args(subparser)
    subparser.set_defaults(func=glue_crawler.cli)

    import dashboard
    subparser = metric_subparsers.add_parser('dashboard', help='View dashboard metrics.')
    subparser.add_argument('--function', required=True, default='get', help='Get the specific key')    
    subparser.add_argument('--param', required=True, default='get', help='The specific key to get from the database')    
    add_common_args(subparser)
    subparser.set_defaults(func=dashboard.cli)


def add_modules():
    sys.path.append('./Gems/CloudGemMetric/v1/AWS/lambda-code/FIFOConsumer')
    sys.path.append('./Gems/CloudGemMetric/v1/AWS/lambda-code/FIFOProducer')
    sys.path.append('./Gems/CloudGemMetric/v1/AWS/lambda-code/ServiceLambda')
    sys.path.append('./Gems/CloudGemMetric/v1/AWS/lambda-code/GlueCrawlerLauncher')
    sys.path.append('./Gems/CloudGemMetric/v1/AWS/lambda-code/ServiceLambda/api')    
    sys.path.append('./Gems/CloudGemFramework/v1/AWS/common-code/LambdaService')   
    sys.path.append('./Gems/CloudGemMetric/v1/AWS/common-code/AWSCommon')
    sys.path.append('./Gems/CloudGemMetric/v1/AWS/common-code/ParquetUtils')
    sys.path.append('./Gems/CloudGemMetric/v1/AWS/common-code/MetricUtils')
    sys.path.append('./Gems/CloudGemMetric/v1/AWS/common-code/MemoryUtils')
    sys.path.append('./Gems/CloudGemMetric/v1/AWS/common-code/Constant')
    sys.path.append('./Gems/CloudGemMetric/v1/AWS/python/windows/Lib')
    sys.path.append('./Gems/CloudGemMetric/v1/AWS/lambda-code/LambdaConsumerLauncher')
    sys.path.append('./Gems/CloudGemMetric/v1/AWS/lambda-code/AmoebaLauncher')
    sys.path.append('./Gems/CloudGemMetric/v1/AWS/lambda-function-code')
    sys.path.append('./Gems/CloudGemMetric/v1/AWS/project-code/lambda-code/CustomResource/resource_types')
    sys.path.append('./Gems/CloudGemFramework/v1/AWS/common-code/Utils')
    

    #add the windows python libs.  The lib in common-code are built for Lambdas linux system
    sys.path.append('./Gems/CloudGemMetric/v1/AWS/python/windows/Lib')
    idx = -1
    for path in sys.path:
        if 'Lib' in path and 'common-code' in path:
            idx = sys.path.index(path)            
    if idx >=0 :
        del sys.path[idx]

    

