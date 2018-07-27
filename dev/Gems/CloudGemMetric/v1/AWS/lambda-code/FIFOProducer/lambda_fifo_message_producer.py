from dynamodb import DynamoDb
from sqs import Sqs
from StringIO import StringIO
from aws_lambda import Lambda
from cgf_utils import custom_resource_response
from thread_pool import ThreadPool
from resource_manager_common import stack_info
from compression import Compress, NoCompression, CompressionClassFactory
from payload import CSV, JSON, PayloadClassFactory
from sensitivity import SensitivityClassFactory
from data_generator import DataGenerator
import os
import boto3
import time
import threading
import collections
import metric_constant as c
import traceback
import random
import util
import errors
import sys
import sqs
import json
import payload
import compression
import sensitivity
import datetime

context = None
aws_sqs = None
timestamp = datetime.datetime.utcnow() 

def main(event, lambdacontext):
    global context
    global timestamp
    global aws_sqs
    start = time.time()
    ok_response =  {        
        'StatusCode': 200,            
    }
    refreshtime = datetime.datetime.utcnow() - datetime.timedelta(minutes=1)
    if context is None or aws_sqs is None or refreshtime > timestamp:        
        context=dict({})    
        stack_id = os.environ[c.ENV_DEPLOYMENT_STACK_ARN]
        context[c.KEY_REQUEST_ID] = lambdacontext.aws_request_id if hasattr(lambdacontext, 'aws_request_id') else None
        db = DynamoDb(context)
        prefix = util.get_stack_name_from_arn(stack_id)
        aws_sqs = Sqs(context, queue_prefix=prefix) 
        aws_sqs.set_queue_url(True)         
        timestamp = datetime.datetime.utcnow() 
    else:
        context[c.KEY_SQS_QUEUE_URL] = aws_sqs.queue_url

    data =  event.get(c.API_PARAM_PAYLOAD, {})[c.API_PARAM_DATA]
    source_IP = event.get(c.API_PARAM_SOURCE_IP, None)     
    sensitivity_type = event.get(c.SQS_PARAM_SENSITIVITY_TYPE, sensitivity.SENSITIVITY_TYPE.NONE)           
    compression_mode = event.get(c.SQS_PARAM_COMPRESSION_TYPE, compression.COMPRESSION_MODE.NONE)           
    payload_type = event.get(c.SQS_PARAM_PAYLOAD_TYPE, payload.PAYLOAD_TYPE.CSV)    
    compression_mode = CompressionClassFactory.instance(compression_mode)    
    sensitivity_type = SensitivityClassFactory.instance(sensitivity_type)
    payload_type = PayloadClassFactory.instance(context, payload_type, compression_mode, sensitivity_type, source_IP)
      
    print "[{}]Using SQS queue URL '{}'".format(context[c.KEY_REQUEST_ID],aws_sqs.queue_url) 
    if os.environ[c.ENV_VERBOSE]== "True":
        print "The post request contains a paylod of\n{}".format(data)
    if data is None:   
        print "Terminating, there is no data."
        return ok_response
        
    total_metrics = "all"    
    try:
        data_size = len(data) + sqs.message_overhead_size(sensitivity_type, compression_mode, payload_type)      
        message_chunks, total_metrics = payload_type.chunk(data)   
    
        for message in message_chunks:                    
            print "Sending a sqs message with {} bytes".format(len(message))                
            aws_sqs.send_message(sensitivity_type, message, compression_mode, payload_type)    
    except Exception as e:        
        traceback.print_exc()                
        raise errors.ClientError(e.message)     

    print "The job sent {} metric(s) to the FIFO queue '{}'".format(total_metrics, aws_sqs.queue_url)    
    print "The job took {} seconds.".format(time.time() -start)
    return ok_response

def ip_address_network(ip_address):
    if "." in ip_address:
        return ip_address_parse_network(ip_address, ".")
    if ":" in ip_address:
        return ip_address_parse_network(ip_address, ":")
    return ip_address

def ip_address_parse_network(ip_address, sep):
    parts = ip_address.split(sep)    
    
