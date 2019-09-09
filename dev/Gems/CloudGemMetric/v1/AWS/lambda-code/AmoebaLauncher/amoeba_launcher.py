from dynamodb import DynamoDb
from aws_lambda import Lambda
from cgf_utils import custom_resource_response
from StringIO import StringIO
from athena import Athena, Query
from datetime import date, timedelta
from sqs import Sqs
from thread_pool import ThreadPool
import os
import metric_constant as c
import util  
import random
import math
import datetime
import metric_schema
import time
import json
import sys

def launch(event, lambdacontext):
    util.debug_print("Start Amoeba Launcher")    
    context = dict({})                 
    context[c.KEY_START_TIME] = time.time() 
    context[c.KEY_LAMBDA_FUNCTION] = lambdacontext.function_name if hasattr(lambdacontext, 'function_name') else None   
    context[c.KEY_REQUEST_ID] = lambdacontext.aws_request_id if hasattr(lambdacontext, 'aws_request_id') else None        
    prefix = util.get_stack_name_from_arn(os.environ[c.ENV_DEPLOYMENT_STACK_ARN])    
    prefix = "{0}{1}".format(prefix, c.KEY_SQS_AMOEBA_SUFFIX)
    db = DynamoDb(context)
    sqs = Sqs(context, prefix, "sqs")
    sqs.set_queue_url(lowest_load_queue=False)    

    if sqs.is_all_under_load:        
        sqs.add_fifo_queue(prefix)   
        
    elapsed = util.elapsed(context) 
    timeout = context[c.KEY_MAX_LAMBDA_TIME] * c.RATIO_OF_MAX_LAMBDA_TIME
    map = {}
    queues_checked = 0
    number_of_queues = sqs.number_of_queues
    sqs_delete_tokens = {}    
    while elapsed < timeout and queues_checked < number_of_queues:    
        messages = sqs.read_queue()  
        length = len(messages)
        if sqs.queue_url not in sqs_delete_tokens:
            sqs_delete_tokens[sqs.queue_url] = []

        if length > 0:            
            for x in range(0, length):
                message = messages[x]   
                body = json.loads(message["Body"])
                paths = body["paths"]
                msg_token = "{}{}{}".format(message['MessageId'], context[c.KEY_SEPERATOR_CSV], message['ReceiptHandle'])
                sqs_delete_tokens[sqs.queue_url].append(msg_token)
                for i in range(0, len(paths)):
                    path = paths[i]
                    parts = path.split(context[c.KEY_SEPERATOR_PARTITION])
                    filename = parts.pop()
                    directory = context[c.KEY_SEPERATOR_PARTITION].join(parts)
                    if directory not in map:
                        map[directory] = {
                            "paths": [],
                            "size": 0
                        }
                    #lambda payload limit for Event invocation type  131072
                    sizeof = len(path) + map[directory]["size"]
                    is_invoked =  map[directory].get("invoked", False)
                    if sizeof >= c.MAXIMUM_ASYNC_PAYLOAD_SIZE and not is_invoked:
                        invoke_lambda(context, directory, map[directory]["paths"] )   
                        map[directory] = {
                            "paths": [],
                            "size": 0,
                            "invoked": True
                        }
                    else:
                        map[directory]["paths"].append(path) 
                        map[directory]["size"] = sizeof   
                        
        else:
            queues_checked += 1
            sqs.set_queue_url(lowest_load_queue=False)    
    
        elapsed = util.elapsed(context)         

    #Invoke a amoeba generator for each S3 leaf node
    for directory, settings in map.iteritems(): 
        is_invoked = settings.get("invoked", False)
        #Amoeba's are not designed to have multiple amoebas working against one directory
        #If the Amoeba has already been invoked due to payload size then we requeue the remaining paths
        if is_invoked:
            sqs.send_generic_message(json.dumps({ "paths": settings["paths"] }))
        else:
            invoke_lambda(context, directory, settings["paths"])    
        
    context[c.KEY_THREAD_POOL] = ThreadPool(context, 8) 
    #Delete SQS messages that have been processed
    for key, value in sqs_delete_tokens.iteritems():        
        sqs.delete_message_batch(value, key)

    return custom_resource_response.success_response({"StatusCode": 200}, "*")

def invoke_lambda(context, directory, files):    
    payload = { "directory": directory,
                "files": files,
                "context": context }   
    
    if context[c.KEY_REQUEST_ID]:           
        awslambda = Lambda()        
        util.debug_print("Invoking lambda {} with path '{}'".format(c.ENV_AMOEBA, files))
        response = awslambda.invoke(os.environ[c.ENV_AMOEBA],payload)              
        util.debug_print("Status Code: {} Response Payload: {}".format(response['StatusCode'], response['Payload'].read().decode('utf-8')))
    else:                
        import amoeba_generator        
        amoeba_generator.ingest(payload, dict({}))    

def cli(context, args):    
    util.set_logger(args.verbose)

    from resource_manager_common import constant
    credentials = context.aws.load_credentials()

    resources = util.get_resources(context)
    os.environ[c.ENV_S3_STORAGE] = resources[c.RES_S3_STORAGE]      
    os.environ[c.ENV_DB_TABLE_CONTEXT] = resources[c.RES_DB_TABLE_CONTEXT]              
    os.environ[c.ENV_VERBOSE] = str(args.verbose) if args.verbose else ""
    os.environ[c.ENV_REGION] = context.config.project_region
    os.environ[c.ENV_AMOEBA] = resources[c.RES_AMOEBA]   
    os.environ[c.IS_LOCALLY_RUN] = "True"
    os.environ[c.ENV_DEPLOYMENT_STACK_ARN] = resources[c.ENV_STACK_ID]
    os.environ["AWS_ACCESS_KEY"] = args.aws_access_key if args.aws_access_key else credentials.get(args.profile if args.profile else context.config.user_default_profile, constant.ACCESS_KEY_OPTION)
    os.environ["AWS_SECRET_KEY"] = args.aws_secret_key if args.aws_secret_key else credentials.get(args.profile if args.profile else context.config.user_default_profile, constant.SECRET_KEY_OPTION)
    launch({}, type('obj', (object,), {}))    