from s3crawler import Crawler
from dynamodb import DynamoDb
from thread_pool import ThreadPool
from threading import Thread
from aws_lambda import Lambda
from cgf_utils import custom_resource_response
from StringIO import StringIO
from athena import Athena, Query
from glue import Glue
from datetime import date, timedelta
import os
import metric_constant as c
import util  
import random
import math
import datetime
import metric_schema

def launch(event, lambdacontext):
    print "Start"
    hours_delta = 36
    context = dict({})                 
    context[c.KEY_LAMBDA_FUNCTION] = lambdacontext.function_name if hasattr(lambdacontext, 'function_name') else None   
    context[c.KEY_REQUEST_ID] = lambdacontext.aws_request_id if hasattr(lambdacontext, 'aws_request_id') else None
    global threadpool
    global is_lambda 
    threadpool = ThreadPool(context, 8)         
    is_lambda = context[c.KEY_REQUEST_ID] is not None    
    available_amoeba_lambdas = []
    available_amoeba_lambdas.append(c.ENV_AMOEBA_1)
    available_amoeba_lambdas.append(c.ENV_AMOEBA_2)
    available_amoeba_lambdas.append(c.ENV_AMOEBA_3)
    available_amoeba_lambdas.append(c.ENV_AMOEBA_4)
    available_amoeba_lambdas.append(c.ENV_AMOEBA_5)
    db = DynamoDb(context)  
    crawler = Crawler(context, os.environ[c.ENV_S3_STORAGE])    
    glue = Glue()
    
    events = glue.get_events()
    #TODO: adjust the amoeba tree depth so that we have fully utilized all available amoebas; len(available_amoeba_lambdas) * 1000
    #since the number of leaf nodes for the metric partitions can quickly get very large we use a 5 lambda pool to ensure we don't hit the 1000 invocation limit.
    
    start = datetime.datetime.utcnow() - datetime.timedelta(hours=hours_delta)
    now = datetime.datetime.utcnow()    
        
    for type in events:        
        dt = start
        while dt <= now:                                                          
            prefix = metric_schema.s3_key_format().format(context[c.KEY_SEPERATOR_PARTITION], dt.year, dt.month, dt.day, dt.hour, type, dt.strftime(util.partition_date_format()))    
            threadpool.add(crawler.crawl, prefix, available_amoeba_lambdas, invoke_lambda)                 
            dt += timedelta(hours=1)
    
    threadpool.wait()    
    return custom_resource_response.success_response({"StatusCode": 200}, "*")

def invoke_lambda(context, root, name):    
    payload = { "root": root,
                "context": context }    
    if is_lambda:                        
        awslambda = Lambda()        
        util.debug_print("Invoking lambda {} with path '{}'".format(name, root))
        awslambda.invoke(os.environ[name],payload)            
    else:                        
        import amoeba_generator
        threadpool.add(amoeba_generator.ingest, payload, dict({}))    

def cli(context, args):    
    resources = util.get_resources(context)
    os.environ[c.ENV_S3_STORAGE] = resources[c.RES_S3_STORAGE]      
    os.environ[c.ENV_DB_TABLE_CONTEXT] = resources[c.RES_DB_TABLE_CONTEXT]              
    os.environ[c.ENV_VERBOSE] = str(args.verbose) if args.verbose else ""
    os.environ[c.ENV_REGION] = context.config.project_region
    os.environ[c.IS_LOCALLY_RUN] = "True"
    os.environ[c.ENV_DEPLOYMENT_STACK_ARN] = resources[c.ENV_STACK_ID]
    launch({}, dict({'aws_request_id':123}))    