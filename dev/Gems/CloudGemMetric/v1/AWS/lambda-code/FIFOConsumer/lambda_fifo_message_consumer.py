from thread_pool import ThreadPool
from aggregator import Aggregator
from dynamodb import DynamoDb
from sqs import Sqs
from aws_lambda import Lambda
from threading import Thread
from cloudwatch import CloudWatch
from glue import Glue
import metric_constant as c
import time
import s3fs
import util
import mem_util as mutil
import traceback
import datetime
import os
import parquet_writer as writer
import json
import athena
import gc
import copy

glue_crawler_response = None
"""
Main entry point for the scheduled lambda

Lambdas triggered by growth > threshold use the process function as the entry point
"""
def main(event, lambdacontext):  
    starttime = time.time()    
    queue_url = event.get(c.KEY_SQS_QUEUE_URL, None)        
    print "Started consumer with queue url '{}'".format(queue_url)    
    context = event.get("context", {})        
    context[c.KEY_SQS_QUEUE_URL] = queue_url        
    context[c.KEY_LAMBDA_FUNCTION] = lambdacontext.function_name if hasattr(lambdacontext, 'function_name') else None
    context[c.KEY_REQUEST_ID] = lambdacontext.aws_request_id if hasattr(lambdacontext, 'aws_request_id') else None
    context[c.KEY_IS_LAMBDA_ENV] = context[c.KEY_REQUEST_ID] is not None
      
    prefix = util.get_stack_name_from_arn(os.environ[c.ENV_DEPLOYMENT_STACK_ARN])    

    context[c.KEY_STACK_PREFIX] = prefix
    context[c.KEY_SQS] = Sqs(context, "{0}_".format(prefix))
    context[c.KEY_SQS_AMOEBA] = Sqs(context, "{0}{1}_".format(prefix, c.KEY_SQS_AMOEBA_SUFFIX))
    context[c.KEY_SQS_AMOEBA].set_queue_url(lowest_load_queue=True)    
    context[c.KEY_LAMBDA] = Lambda(context)
    context[c.KEY_CLOUDWATCH] = CloudWatch(context)
    
    context[c.KEY_THREAD_POOL] = ThreadPool(context, 8)               
    context[c.KEY_METRIC_BUCKET] = os.environ[c.RES_S3_STORAGE]            
    
    context[c.KEY_START_TIME] = starttime
    context[c.CW_ATTR_SAVE_DURATION] = context[c.KEY_CLOUDWATCH].avg_save_duration(util.get_cloudwatch_namespace(os.environ[c.ENV_DEPLOYMENT_STACK_ARN]))
    context[c.CW_ATTR_DELETE_DURATION] = context[c.KEY_CLOUDWATCH].avg_delete_duration(util.get_cloudwatch_namespace(os.environ[c.ENV_DEPLOYMENT_STACK_ARN]))    
          
    context[c.KEY_SUCCEEDED_MSG_IDS] = []
    process(context)    
    del context
    gc.collect()
    return {        
        'StatusCode': 200        
    }

def process(context):    
    print mutil.get_memory_object() 
    write_initial_stats(context)            
    process_bytes = mutil.get_process_memory_usage_bytes()
    if c.KEY_SQS_QUEUE_URL not in context or context[c.KEY_SQS_QUEUE_URL] is None:
        context[c.KEY_SQS].set_queue_url(lowest_load_queue=False)
    #execute at least once
    messages_to_process = None     
    inflight_messages = 0
    elapsed = 0
    metric_sets = dict({})    
    context[c.KEY_AGGREGATOR] = Aggregator(context, metric_sets)   
    messages = []
    last_queue_size_check = context[c.KEY_FREQUENCY_TO_CHECK_TO_SPAWN_ANOTHER]
    growth_rate = last_check = 0
    last_message_count = None   
    timeout = calculate_aggregate_window_timeout(context)
    value = datetime.datetime.fromtimestamp(context[c.KEY_START_TIME])
    message_processing_time = 0
    print "[{}]Using SQS queue URL '{}'".format(context[c.KEY_REQUEST_ID],context[c.KEY_SQS].queue_url) 
    print "[{}]Started the consumer at {}.  The aggregation window is {} seconds.".format(context[c.KEY_REQUEST_ID],value.strftime('%Y-%m-%d %H:%M:%S'), timeout)
    while elapsed < timeout: 
        if elapsed > last_check:  
            last_check = elapsed + context[c.KEY_FREQUENCY_TO_CHECK_SQS_STATE]                        
            response=context[c.KEY_SQS].get_queue_attributes()
            inflight_messages = int(response['Attributes']['ApproximateNumberOfMessagesNotVisible']) 
            messages_to_process = int(response['Attributes']['ApproximateNumberOfMessages'])                             
            if last_message_count is None:            
                last_message_count = messages_to_process            
            else:
                growth_rate = last_message_count if last_message_count == 0 else float(messages_to_process - last_message_count) / last_message_count            
                last_message_count = messages_to_process                
                grow_if_threshold_hit(context, growth_rate, context[c.KEY_GROWTH_RATE_BEFORE_ADDING_LAMBDAS])        
            
            #if the queue is growing slowly and is above 30,000 messages launch a new consumer
            if elapsed > last_queue_size_check:
                last_queue_size_check = elapsed + context[c.KEY_FREQUENCY_TO_CHECK_TO_SPAWN_ANOTHER]                
                print "[{}]\nThere are approximately {} messages that require processing.\n" \
                    "There are {} in-flight messages.\n" \
                    "{} seconds have elapsed and there is {} seconds remaining before timeout.\n" \
                    "The queue growth rate is {}\n" \
                    "{} message(s) were processed.".format(context[c.KEY_REQUEST_ID], messages_to_process,inflight_messages,round(elapsed,2),util.time_remaining(context),growth_rate,len(messages))                            
                if messages_to_process > context[c.KEY_THRESHOLD_BEFORE_SPAWN_NEW_CONSUMER] and inflight_messages <= context[c.KEY_MAX_INFLIGHT_MESSAGES]:
                    print "The queue size is greater than {}. Launching another consumer.".format(context[c.KEY_THRESHOLD_BEFORE_SPAWN_NEW_CONSUMER])
                    add_consumer(context)
            if last_message_count == 0: 
                print "[{}]No more messages to process.".format(context[c.KEY_REQUEST_ID])          
                break;
        messages = context[c.KEY_SQS].read_queue()                 
                               
        if len(messages) > 0:
            start = time.time()   
            context[c.KEY_AGGREGATOR].append_default_metrics_and_partition(messages)  
            message_processing_time = round(((time.time() - start) + message_processing_time) / 2, 4)            
        else:
            if len(metric_sets) > 1:
                print "[{}]No more messages to process.".format(context[c.KEY_REQUEST_ID])          
                break
            else:
                print "[{}]No metric sets to process. Exiting.".format(context[c.KEY_REQUEST_ID])          
                return;    
                
        #start throttling the message processing when the SQS inflight messages is at 80% (16,000)
        #one queue is only allowed to have 20,000 maximum messages being processed (in-flight)
        usage = mutil.get_memory_usage()        
        if inflight_messages > 16000:
            print "[{}]Stopping aggregation.  There are too many messages in flight.  Currently there are {} messages in flight.".format(context[c.KEY_REQUEST_ID],inflight_messages)           
            break
        if usage > context[c.KEY_MEMORY_FLUSH_TRIGGER]:             
            print "[{}]Stopping aggregation.  Memory safe level threshold exceeded.  The lambda is currently at {}%.".format(context[c.KEY_REQUEST_ID],usage)           
            break
        if util.elapsed(context) + message_processing_time > timeout:             
            print "[{}]Stopping aggregation.  The elapsed time and the projected message processing time exceeds the timeout window.  Messages are taking {} seconds to process.  There is {} seconds left before time out and {} seconds for aggregation.".format(context[c.KEY_REQUEST_ID],message_processing_time,util.time_remaining(context),timeout)                       
            break
          
        elapsed = util.elapsed(context)        

    util.debug_print("[{}]Lambda has completed the agreggation phase.  Elapsed time was {} seconds and we have {} seconds remaining. There are {} in-flight messages and {} remaining messages to process.".format(context[c.KEY_REQUEST_ID],elapsed, util.time_remaining(context), inflight_messages, messages_to_process))
    context[c.KEY_THREAD_POOL].wait() 
    bytes_consumed = mutil.get_process_memory_usage_bytes()
    memory_usage = str(mutil.get_memory_usage())
    print mutil.get_memory_object()    
    tables = metric_sets[c.KEY_TABLES]
    del metric_sets[c.KEY_TABLES]
    flush_and_delete(context, metric_sets)
    context[c.KEY_THREAD_POOL].wait() 
    update_glue_crawler_datastores(context, tables)                     
    print mutil.get_memory_object()
    print "[{}]Elapsed time {} seconds. ".format(context[c.KEY_REQUEST_ID],util.elapsed(context))
    print "[{}]Message processing averaged {} seconds per message. ".format(context[c.KEY_REQUEST_ID], message_processing_time)
    print "[{}]The process consumed {} KB of memory.".format(context[c.KEY_REQUEST_ID],bytes_consumed/1024)
    print '[{}]The memory utilization was at {}%.'.format(context[c.KEY_REQUEST_ID],memory_usage)
    print '[{}]The process used {} KB for converting messages to parquet format.'.format(context[c.KEY_REQUEST_ID],(bytes_consumed - process_bytes)/1024 )
    print "[{}]The save process took {} seconds.".format(context[c.KEY_REQUEST_ID],context[c.CW_ATTR_SAVE_DURATION])          
    print "[{}]Processed {} uncompressed bytes.".format(context[c.KEY_REQUEST_ID],context[c.KEY_AGGREGATOR].bytes_uncompressed)
    print "[{}]Processed {} metrics. ".format(context[c.KEY_REQUEST_ID],context[c.KEY_AGGREGATOR].rows)
    print "[{}]Processed {} messages. ".format(context[c.KEY_REQUEST_ID],context[c.KEY_AGGREGATOR].messages)
    print "[{}]Average metrics per minute {}. ".format(context[c.KEY_REQUEST_ID],round(context[c.KEY_AGGREGATOR].rows / util.elasped_time_in_min(context), 2))
    print "[{}]Average messages per minute {}. ".format(context[c.KEY_REQUEST_ID],round(context[c.KEY_AGGREGATOR].messages / util.elasped_time_in_min(context),2))
    print "[{}]Average uncompressed bytes per minute {}. ".format(context[c.KEY_REQUEST_ID],round(context[c.KEY_AGGREGATOR].bytes_uncompressed / util.elasped_time_in_min(context),2))
    print "[{}]There are approximately {} messages that require processing.".format(context[c.KEY_REQUEST_ID],messages_to_process if messages_to_process else 0)
    print "[{}]There are {} in-flight messages.".format(context[c.KEY_REQUEST_ID],inflight_messages)            
    print "[{}]There was {} seconds remaining before timeout. ".format(context[c.KEY_REQUEST_ID],util.time_remaining(context))          
    del tables
    del metric_sets
    gc.collect()

def calculate_aggregate_window_timeout(context):
    maximum_lambda_duration = context[c.KEY_MAX_LAMBDA_TIME] 
    maximum_aggregation_period = maximum_lambda_duration * c.RATIO_OF_MAX_LAMBDA_TIME     
    requested_aggregation_window_timeout = context[c.KEY_AGGREGATION_PERIOD_IN_SEC] if context[c.KEY_AGGREGATION_PERIOD_IN_SEC] < maximum_aggregation_period else maximum_aggregation_period
    delta = maximum_lambda_duration - requested_aggregation_window_timeout
    avg_time_to_save = context[c.CW_ATTR_SAVE_DURATION]     
    avg_time_to_delete = context[c.CW_ATTR_DELETE_DURATION] 
    
    print "[{}]The requested aggregation period is {} seconds".format(context[c.KEY_REQUEST_ID],context[c.KEY_AGGREGATION_PERIOD_IN_SEC])    
    print "[{}]The maximum allowable aggregation period (max lambda time * {}) is {} seconds".format(context[c.KEY_REQUEST_ID],c.RATIO_OF_MAX_LAMBDA_TIME, maximum_aggregation_period)    
    print "[{}]The S3 flush process has been averaging {} seconds.".format(context[c.KEY_REQUEST_ID],avg_time_to_save + avg_time_to_delete)
    
    #it is taking to long to save.  Set the aggregation window as the minimum
    if avg_time_to_save > 0 and \
            avg_time_to_delete >= 0 and \
            avg_time_to_save + avg_time_to_delete >= requested_aggregation_window_timeout:
        return 15 #minimum save window size, one iteration
    #not enough data to provide an estimated save window, use the default
    elif avg_time_to_save <= 0 and avg_time_to_delete <= 0: 
        print "[{}]There is not enough data to use the previous save durations.  Setting the aggregation window to half of {} seconds.".format(context[c.KEY_REQUEST_ID],requested_aggregation_window_timeout)    
        return requested_aggregation_window_timeout/2
    
    #save duration exceeds the requested aggregation window        
    return requested_aggregation_window_timeout - avg_time_to_save - avg_time_to_delete

def grow_if_threshold_hit(context, rate, threshold):
    if rate > threshold :
        print "[{}]The current growth rate {} has exceeded threshold {}. Adding a new consumer lambda.".format(context[c.KEY_REQUEST_ID],rate, threshold)                 
        add_consumer(context)
        gc.collect()

def add_consumer(context):           
    threadpool = context[c.KEY_THREAD_POOL]      
    payload = { c.KEY_SQS_QUEUE_URL: context[c.KEY_SQS_QUEUE_URL], "context": copy_context(context) }    
    if context[c.KEY_IS_LAMBDA_ENV]:       
        threadpool.add(context[c.KEY_LAMBDA].invoke, context[c.KEY_LAMBDA_FUNCTION], payload)
    else: 
        main(payload,type('obj', (object,), {'function_name' : context[c.KEY_LAMBDA_FUNCTION]}))            

def copy_context(context):
    #can't serialize these objects
    context_copy = context.copy()
    if c.KEY_SQS in context_copy:
        del context_copy[c.KEY_SQS]
    if c.KEY_SQS_AMOEBA in context_copy:
        del context_copy[c.KEY_SQS_AMOEBA]
    if c.KEY_LAMBDA in context_copy:
        del context_copy[c.KEY_LAMBDA]
    if c.KEY_CLOUDWATCH in context_copy:
        del context_copy[c.KEY_CLOUDWATCH]    
    if c.KEY_THREAD_POOL in context_copy:
        del context_copy[c.KEY_THREAD_POOL]    
    if c.KEY_AGGREGATOR in context_copy:
        del context_copy[c.KEY_AGGREGATOR] 
    return context_copy

def flush_and_delete(context, metric_sets):        
    if len(metric_sets)>0:
        print "[{}]Time remaining {} seconds".format(context[c.KEY_REQUEST_ID], util.time_remaining(context))
        print "[{}]Saving aggregated metrics to S3 bucket '{}'".format(context[c.KEY_REQUEST_ID], os.environ[c.RES_S3_STORAGE])
        context[c.CW_ATTR_SAVE_DURATION] = save_duration = flush(context, metric_sets)   
        #delete only messages that have succeeded across all partition saves
        #message payloads can span multiple partitions/schemas
        #only delete messages that succeeded on all partition/schema submissions
        #this could lead to duplication of data if only part of a message is processed successfully.
        #this is better than deleting the message entirely leaving missing data.
        context[c.KEY_THREAD_POOL].wait()
        print "[{}]Time remaining {} seconds".format(context[c.KEY_REQUEST_ID], util.time_remaining(context))
        print "[{}]Deleting messages from SQS queue '{}'".format(context[c.KEY_REQUEST_ID],context[c.KEY_SQS].queue_url)
        delete_duration = context[c.KEY_SQS].delete_message_batch(context[c.KEY_SUCCEEDED_MSG_IDS])
        write_cloudwatch_metrics(context, save_duration, delete_duration)

def update_glue_crawler_datastores(context, datastores):    
    global glue_crawler_response
    glue = Glue()    
    crawler_name = glue.get_crawler_name(context[c.KEY_LAMBDA_FUNCTION])
    if not glue_crawler_response:
        glue_crawler_response = glue.get_crawler(crawler_name)    
    if glue_crawler_response is not None:                
        bucket = "s3://{}/".format(os.environ[c.RES_S3_STORAGE])
        path_format = "s3://{}/{}".format(os.environ[c.RES_S3_STORAGE],"{}")        
        srcs = []
        if len(glue_crawler_response['Crawler']['Targets']['S3Targets']) > 0:
            for s3target in glue_crawler_response['Crawler']['Targets']['S3Targets']:
                table = s3target['Path'].replace(bucket,'').lower()
                if table in datastores:
                    del datastores[table]                
                srcs.append(s3target)                    
        
        if len(datastores) == 0:
            return

        for table in datastores:
            srcs.append({
                    'Path': path_format.format(table),
                    'Exclusions': []
                })
        print "Defining GLUE datastores"
        db_name = athena.get_database_name(os.environ[c.ENV_S3_STORAGE])
        table_prefix = athena.get_table_prefix(os.environ[c.ENV_S3_STORAGE])
        glue.update_crawler(crawler_name, os.environ[c.ENV_SERVICE_ROLE], db_name, table_prefix , srcs=srcs)

def flush(context, metric_sets):    
    start = time.time()     
    #errors in threads write the msgids to the failed msg array so they are not deleted from SQS   
    threadpool = context[c.KEY_THREAD_POOL]    
    for partition, schema_hash in metric_sets.iteritems():
         threadpool.add(writer.save, context, metric_sets, partition, schema_hash)       
    return int(time.time() - start)

def write_cloudwatch_metrics(context, save_duration, delete_duration):    
    cw = context[c.KEY_CLOUDWATCH]    
    cw.put_metric_data(util.get_cloudwatch_namespace(context[c.KEY_LAMBDA_FUNCTION]),
                [
                {
                    "MetricName":c.CW_METRIC_NAME_PROCESSED,
                    "Dimensions":[{'Name':c.CW_METRIC_DIMENSION_NAME_CONSUMER, 'Value': c.CW_METRIC_DIMENSION_ROWS}],
                    "Timestamp":datetime.datetime.utcnow(),
                    "Value":context[c.KEY_AGGREGATOR].rows,                                
                    "Unit":'Count'                                      
                },
                {
                    "MetricName":c.CW_METRIC_NAME_PROCESSED,
                    "Dimensions":[{'Name':c.CW_METRIC_DIMENSION_NAME_CONSUMER, 'Value': c.CW_METRIC_DIMENSION_BYTES}],
                    "Timestamp":datetime.datetime.utcnow(),
                    "Value":context[c.KEY_AGGREGATOR].bytes_uncompressed,                                
                    "Unit":'Bytes'                    
                },
                {
                    "MetricName":c.CW_METRIC_NAME_PROCESSED,
                    "Dimensions":[{'Name':c.CW_METRIC_DIMENSION_NAME_CONSUMER, 'Value': c.CW_METRIC_DIMENSION_MSG}],
                    "Timestamp":datetime.datetime.utcnow(),
                    "Value":context[c.KEY_AGGREGATOR].messages,                                
                    "Unit":'Count'                    
                },
                {
                    "MetricName":c.CW_METRIC_NAME_PROCESSED,
                    "Dimensions":[{'Name':c.CW_METRIC_DIMENSION_NAME_CONSUMER, 'Value': c.CW_METRIC_DIMENSION_SAVE}],
                    "Timestamp":datetime.datetime.utcnow(),
                    "Value":save_duration,                                
                    "Unit":'Seconds'                    
                },
                {
                    "MetricName":c.CW_METRIC_NAME_PROCESSED,
                    "Dimensions":[{'Name':c.CW_METRIC_DIMENSION_NAME_CONSUMER, 'Value': c.CW_METRIC_DIMENSION_DEL}],
                    "Timestamp":datetime.datetime.utcnow(),
                    "Value":delete_duration,                                
                    "Unit":'Seconds'                    
                },
                ]
            )    
    if context.get(c.KEY_WRITE_DETAILED_CLOUDWATCH_EVENTS, False):
        util.debug_print("Sending detailed CloudWatch events")
        write_detailed_cloud_watch_event_logs(context, cw)

def write_detailed_cloud_watch_event_logs(context, cw):
    events = context[c.KEY_AGGREGATOR].events    
    params = []
    for event_name, event_count in events.iteritems():
        params.append(
            {
                "MetricName":c.CW_METRIC_NAME_PROCESSED,
                "Dimensions":[{'Name':c.CW_METRIC_DIMENSION_NAME_CONSUMER, 'Value': event_name}],
                "Timestamp":datetime.datetime.utcnow(),
                "Value": event_count,                                
                "Unit":'Count'                                      
            }
        )                
        if len(params) >= c.CW_MAX_METRIC_SUBMISSIONS:
            cw.put_metric_data(util.get_cloudwatch_namespace(util.get_cloudwatch_namespace(context[c.KEY_LAMBDA_FUNCTION])), params)
            params = []

    if len(params) > 0:
        cw.put_metric_data(util.get_cloudwatch_namespace(util.get_cloudwatch_namespace(context[c.KEY_LAMBDA_FUNCTION])), params)

def write_initial_stats(context):    
    pass

