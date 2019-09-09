from sqs import Sqs
from aws_lambda import Lambda
from threading import Thread
from cgf_utils import custom_resource_response
from dynamodb import DynamoDb
from kms import Kms
import metric_constant as c
import time
import os
import json
import traceback
import util 

"""
Main entry point for the scheduled lambda

Lambdas triggered by growth > threshold use the process function as the entry point
"""
def main(event, lambdacontext):
    context = dict({})
    stack_id = os.environ[c.ENV_DEPLOYMENT_STACK_ARN]
    context[c.KEY_LAMBDA_FUNCTION] = lambdacontext.function_name if hasattr(lambdacontext, 'function_name') else None
    context[c.KEY_REQUEST_ID] = lambdacontext.aws_request_id if hasattr(lambdacontext, 'aws_request_id') else None    
    is_lambda =  context[c.KEY_REQUEST_ID] is not None
    db = DynamoDb(context)
    if not is_lambda:
        import lambda_fifo_message_consumer as consumer
        
    prefix = util.get_stack_name_from_arn(stack_id)
    sqs = Sqs(context, "{0}_".format(prefix))
    awslambda = Lambda(context)
    
    if sqs.is_all_under_load:        
        sqs.add_fifo_queue(prefix)         

    queues = sqs.get_queues()    
    for queue_url in queues:                
        payload = { c.KEY_SQS_QUEUE_URL: queue_url,
                    "context": context}        
        print "Starting {} with queue url '{}'".format("lambda" if is_lambda else "thread", queue_url)        
        if is_lambda:            
            invoke(context, awslambda, payload)
        else:       
            payload[c.ENV_STACK_ID]= event['StackId']              
            consumer.main(payload, type('obj', (object,), {'function_name' : context[c.KEY_LAMBDA_FUNCTION]}))             
        
    print "{} {} lambdas have started".format(len(queues), context[c.KEY_LAMBDA_FUNCTION] )
    return custom_resource_response.success_response({}, "*")

def invoke(context, awslambda, payload):    
    for i in range(0,context[c.KEY_NUMBER_OF_INIT_LAMBDAS]):        
        awslambda.invoke(os.environ[c.ENV_LAMBDA_CONSUMER],payload)

def initialize_s3_kms():
    kms = Kms()    
    if not kms.alias_exists(c.KMS_KEY_ID):        
        key_id = kms.get_key()     
        if key_id is None:
            key_id = kms.create_key()['KeyId']
            kms.create_key_alias(key_id, c.KMS_KEY_ID)
        else:
            kms.create_key_alias(key_id, c.KMS_KEY_ID)


def cli(context, args):    
    util.set_logger(args.verbose)    
    
    from resource_manager_common import constant
    credentials = context.aws.load_credentials()    

    resources = util.get_resources(context)
    os.environ[c.ENV_SHARED_BUCKET] = context.config.configuration_bucket_name
    os.environ[c.ENV_S3_STORAGE] = resources[c.RES_S3_STORAGE]      
    os.environ[c.ENV_DB_TABLE_CONTEXT] = resources[c.RES_DB_TABLE_CONTEXT]              
    os.environ[c.ENV_VERBOSE] = str(args.verbose) if args.verbose else ""
    os.environ[c.ENV_SERVICE_ROLE] = resources[c.RES_SERVICE_ROLE]
    os.environ[c.ENV_REGION] = context.config.project_region
    os.environ[c.ENV_DEPLOYMENT_STACK_ARN] = resources[c.ENV_STACK_ID]
    os.environ[c.ENV_EVENT_EMITTER] = resources[c.RES_EVENT_EMITTER]
    os.environ[c.IS_LOCALLY_RUN] = "True"
    os.environ["AWS_ACCESS_KEY"] = args.aws_access_key if args.aws_access_key else credentials.get(args.profile if args.profile else context.config.user_default_profile, constant.ACCESS_KEY_OPTION)
    os.environ["AWS_SECRET_KEY"] = args.aws_secret_key if args.aws_secret_key else credentials.get(args.profile if args.profile else context.config.user_default_profile, constant.SECRET_KEY_OPTION)
    main({c.ENV_STACK_ID:resources[c.ENV_STACK_ID]}, type('obj', (object,), {}))


