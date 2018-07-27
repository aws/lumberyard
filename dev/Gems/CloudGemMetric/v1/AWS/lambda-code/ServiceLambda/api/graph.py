
import service
import os
import metric_constant as c
import errors
import datetime
import util
from dynamodb import DynamoDb
from cloudwatch import CloudWatch
from athena import Athena, Query

@service.api
def consumer_save_duration(request):    
    cw = CloudWatch()    
    return  cw.get_metric(util.get_cloudwatch_namespace(os.environ[c.ENV_DEPLOYMENT_STACK_ARN]), c.CW_METRIC_NAME_PROCESSED,  c.CW_METRIC_DIMENSION_NAME_CONSUMER, c.CW_METRIC_DIMENSION_SAVE, "Average", start=(datetime.datetime.today() - datetime.timedelta(hours=8)))            

@service.api
def consumer_messages_processed(request):    
    cw = CloudWatch()
    return  cw.get_metric(util.get_cloudwatch_namespace(os.environ[c.ENV_DEPLOYMENT_STACK_ARN]), c.CW_METRIC_NAME_PROCESSED,  c.CW_METRIC_DIMENSION_NAME_CONSUMER, c.CW_METRIC_DIMENSION_MSG, "Average", start=(datetime.datetime.today() - datetime.timedelta(hours=8)))            

@service.api
def consumer_bytes_processed(request):    
    cw = CloudWatch()
    return  cw.get_metric(util.get_cloudwatch_namespace(os.environ[c.ENV_DEPLOYMENT_STACK_ARN]), c.CW_METRIC_NAME_PROCESSED,  c.CW_METRIC_DIMENSION_NAME_CONSUMER, c.CW_METRIC_DIMENSION_BYTES, "Average", start=(datetime.datetime.today() - datetime.timedelta(hours=8)))            

@service.api
def consumer_rows_added(request):    
    cw = CloudWatch()
    return  cw.get_metric(util.get_cloudwatch_namespace(os.environ[c.ENV_DEPLOYMENT_STACK_ARN]), c.CW_METRIC_NAME_PROCESSED,  c.CW_METRIC_DIMENSION_NAME_CONSUMER, c.CW_METRIC_DIMENSION_ROWS, "Average", start=(datetime.datetime.today() - datetime.timedelta(hours=8)))            

@service.api
def consumer_sqs_delete_duration(request):    
    cw = CloudWatch()
    return cw.get_metric(util.get_cloudwatch_namespace(os.environ[c.ENV_DEPLOYMENT_STACK_ARN]), c.CW_METRIC_NAME_PROCESSED,  c.CW_METRIC_DIMENSION_NAME_CONSUMER, c.CW_METRIC_DIMENSION_DEL, "Average", start=(datetime.datetime.today() - datetime.timedelta(hours=8)))            

@service.api
def cloudwatch_metric(request, namespace, metric_name, dimension_name, dimension_value, aggregation_type, time_delta_hours=8, period_in_seconds=300):    
    if dimension_value not in os.environ:
        raise errors.ClientError("The dimension value '{}' is not one of the environment variables.  It should be the logical name of a resource.  Example: FIFOConsumer.".format(dimension_value))     

    cw = CloudWatch()
    return  cw.get_metric("AWS/"+namespace, metric_name, dimension_name, os.environ[dimension_value], aggregation_type, start=(datetime.datetime.today() - datetime.timedelta(hours=time_delta_hours)), period=period_in_seconds)    
   
@service.api
def avg_save_duration(request):
    cw = CloudWatch()
    return cw.avg_save_duration(util.get_cloudwatch_namespace(os.environ[c.ENV_DEPLOYMENT_STACK_ARN]))

@service.api
def avg_delete_duration(request):
    cw = CloudWatch()
    return cw.avg_delete_duration(util.get_cloudwatch_namespace(os.environ[c.ENV_DEPLOYMENT_STACK_ARN]))
   
@service.api
def sum_save_duration(request):
    cw = CloudWatch()
    return cw.sum_save_duration(util.get_cloudwatch_namespace(os.environ[c.ENV_DEPLOYMENT_STACK_ARN]))

@service.api
def sum_delete_duration(request):
    cw = CloudWatch()
    return cw.sum_delete_duration(util.get_cloudwatch_namespace(os.environ[c.ENV_DEPLOYMENT_STACK_ARN]))

def cli(context, args):        
    resources = util.get_resources(context)        
    os.environ[c.ENV_DB_TABLE_CONTEXT] = resources[c.RES_DB_TABLE_CONTEXT]   
    os.environ[c.ENV_DEPLOYMENT_STACK_ARN] = resources[c.RES_LAMBDA_FIFOCONSUMER]   
    os.environ[c.ENV_LAMBDA_PRODUCER] = resources[c.RES_LAMBDA_FIFOPRODUCER]   
    os.environ[c.ENV_AMOEBA_1] = resources[c.RES_AMOEBA_1]   
    os.environ[c.ENV_AMOEBA_2] = resources[c.RES_AMOEBA_2]   
    os.environ[c.ENV_AMOEBA_3] = resources[c.RES_AMOEBA_3]   
    os.environ[c.ENV_AMOEBA_4] = resources[c.RES_AMOEBA_4]   
    os.environ[c.ENV_AMOEBA_5] = resources[c.RES_AMOEBA_5]   
    os.environ[c.ENV_VERBOSE] = str(args.verbose) if args.verbose else ""
    os.environ[c.ENV_REGION] = context.config.project_region
    os.environ[c.ENV_S3_STORAGE] = resources[c.RES_S3_STORAGE]  
    
    print eval(args.function)( type('obj', (object,), {c.ENV_STACK_ID: resources[c.ENV_STACK_ID]}))
