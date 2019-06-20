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

import service
import os
import metric_constant as c
import errors
import util
import time
import compression
import sensitivity
import random
import json 
from dynamodb import DynamoDb
from thread_pool import ThreadPool
from cgf_utils import custom_resource_response
from aws_lambda import Lambda

@service.api
def message(request, compression_mode, sensitivity_type, payload_type, data):    
    p_lambda = Lambda({})    
    print "Target lambda {}".format(os.environ[c.ENV_LAMBDA_PRODUCER])
    util.debug_print("Invoking lambda {} with payload size: {} Compression mode: {}, Sensitivity Type: {}, Payload Type: {}".format(os.environ[c.ENV_LAMBDA_PRODUCER], len(data), compression_mode, sensitivity_type,  payload_type))
    payload_data = {c.API_PARAM_SOURCE_IP:request.event[c.API_PARAM_SOURCE_IP], c.SQS_PARAM_SENSITIVITY_TYPE:  sensitivity_type, c.SQS_PARAM_PAYLOAD_TYPE:  payload_type, c.SQS_PARAM_COMPRESSION_TYPE:  compression_mode, c.API_PARAM_PAYLOAD : data }       
    
    response = p_lambda.invoke_sync(os.environ[c.ENV_LAMBDA_PRODUCER], payload_data)        
    
    sb = response['Payload']
    sb = json.loads(sb.read().decode("utf-8"))
    error = sb.get('errorMessage', None)
    
    returnObj={        
        'StatusCode': response['StatusCode'],        
        'PhysicalResourceId': os.environ[c.ENV_LAMBDA_PRODUCER]    
    }

    if error and len(error) > 0:
        print "Error:", sb
        raise errors.ClientError("An error occurred invoking the SQS event producer.  Please check the cloud watch logs.");

    return returnObj

def thread_job(functionid, iterations_per_thread, events_per_iteration, use_lambda, context, sleep_duration, event_type, sensitivity_type, compression_mode):
    #Module is not available in a lambda environment            
    from data_generator import DataGenerator
    import payload
    import lambda_fifo_message_producer
    p_lambda = Lambda({})    
    data_generator = DataGenerator(context)   
    #data = data_generator.csv(events_per_iteration, event_type) 
    for t in range(0, iterations_per_thread):            
        if sensitivity_type == None:                            
            sensitivity_type = random.choice([sensitivity.SENSITIVITY_TYPE.ENCRYPT])                
        if compression_mode == None:                            
            compression_mode = random.choice([compression.COMPRESSION_MODE.NONE, compression.COMPRESSION_MODE.COMPRESS])        
        payload_type = random.choice([payload.PAYLOAD_TYPE.CSV, payload.PAYLOAD_TYPE.JSON])
        data = None
        #TODO: test data generation should be moved to the payload sub class
        if payload_type == payload.PAYLOAD_TYPE.JSON:
            data = data_generator.json(events_per_iteration, event_type)
        else:
            data = data_generator.csv(events_per_iteration, event_type)       

        if os.environ[c.ENV_VERBOSE]:
            print "Data: \t{}".format(os.environ[c.ENV_VERBOSE]) 

        if use_lambda:            
            response = message(type('obj', (object,), {'event' :{c.API_PARAM_SOURCE_IP:'127.0.0.1'}}), compression_mode, sensitivity_type, payload_type, {c.API_PARAM_DATA: data})            
        else:          
            payload_data = {c.API_PARAM_SOURCE_IP:'127.0.0.1', c.SQS_PARAM_SENSITIVITY_TYPE:  sensitivity_type, c.SQS_PARAM_PAYLOAD_TYPE:  payload_type, c.SQS_PARAM_COMPRESSION_TYPE:  compression_mode, c.API_PARAM_PAYLOAD : {c.API_PARAM_DATA: data} }
            response = lambda_fifo_message_producer.main(payload_data, type('obj', (object,), {}))
        print "StatusCode: {}".format(response['StatusCode'])
        time.sleep(sleep_duration)
        

def generate_threads(functionid, threads_count, iterations_per_thread, events_per_iteration, sleep_duration, use_lambda, event_type, sensitivity_type, compression_mode):
    start = time.time()       
    context = {}            
    threadpool = ThreadPool(context, threads_count)  
    context=dict({})        
    db = DynamoDb(context) 
    print "Sleep durations: ", sleep_duration
    print "Number of threads: ", threads_count
    print "Number of iterations per thread: ", iterations_per_thread
    print "Number of events per iteration: ", events_per_iteration
    print "Using event type: ", event_type
    print "Using sensitivity type: ", sensitivity_type
    print "Using compression mode: ", compression_mode
    for i in range(0, threads_count):          
        threadpool.add(thread_job, functionid, iterations_per_thread, events_per_iteration, use_lambda, context, sleep_duration, event_type, sensitivity_type, compression_mode)                                                    
    threadpool.wait()      
    print "A total of {} metrics have been sent to the FIFO queues.".format((iterations_per_thread*events_per_iteration)*threads_count)    
    print "The overall process took {} seconds.".format(time.time() - start)

def cli(context, args):
    util.set_logger(args.verbose)

    from resource_manager_common import constant
    credentials = context.aws.load_credentials()

    resources = util.get_resources(context)
    os.environ[c.ENV_DB_TABLE_CONTEXT] = resources[c.RES_DB_TABLE_CONTEXT]         
    os.environ[c.ENV_VERBOSE] = str(args.verbose) if args.verbose else ""    
    os.environ['err'] = str(args.erroneous_metrics) if args.erroneous_metrics else ""
    os.environ[c.ENV_REGION] = context.config.project_region    
    os.environ["AWS_LAMBDA_FUNCTION_NAME"] = resources[c.RES_LAMBDA_FIFOPRODUCER]
    os.environ["AWS_ACCESS_KEY"] = args.aws_access_key if args.aws_access_key else credentials.get(args.profile if args.profile else context.config.user_default_profile, constant.ACCESS_KEY_OPTION)
    os.environ["AWS_SECRET_KEY"] = args.aws_secret_key if args.aws_secret_key else credentials.get(args.profile if args.profile else context.config.user_default_profile, constant.SECRET_KEY_OPTION)
    os.environ[c.ENV_LAMBDA_PRODUCER] = resources[c.RES_LAMBDA_FIFOPRODUCER]
    os.environ[c.ENV_DEPLOYMENT_STACK_ARN] = resources[c.ENV_STACK_ID]
    generate_threads( resources[c.RES_LAMBDA_FIFOPRODUCER], args.threads, args.iterations_per_thread, args.events_per_iteration, args.sleep_duration_between_jobs, args.use_lambda, args.event_type, args.sensitivity_type, args.compression_type)