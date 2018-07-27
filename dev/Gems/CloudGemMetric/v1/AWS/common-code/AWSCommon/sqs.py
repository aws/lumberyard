import retry
import boto3
import metric_constant as c
import math
import uuid
import sys
import time
import os
import random
import json
import util

def message_overhead_size(sensitivity_type, compression_type, payload_type):    
    #empty message body + sensitivity type parameter + compression_mode   
    compression = compression_type.identifier
    sensitivity = sensitivity_type.identifier
    payload = payload_type.identifier
    return len(empty_body_message()) + len(payload) + len(sensitivity) + len(compression) + len(message_attributes(sensitivity, compression, payload)) + len(c.SQS_PARAM_MESSAGE_GROUP_ID) + len(c.SQS_PARAM_QUEUE_URL) + len(c.SQS_PARAM_MESSAGE_DEPULICATIONID) + len(c.SQS_PARAM_DELAY_SECONDS) + len(c.SQS_PARAM_MESSAGE_ATTRIBUTES) + 100

def empty_body_message():
    return "N"

def message_attributes(sensitivity_type, compression_mode, payload_type):
    return {
                c.SQS_PARAM_SENSITIVITY_TYPE: {
                        'StringValue': sensitivity_type,
                        'DataType': 'String'
                    },
                c.SQS_PARAM_COMPRESSION_TYPE: {
                        'StringValue': compression_mode,
                        'DataType': 'String'
                    },
                c.SQS_PARAM_PAYLOAD_TYPE: {
                        'StringValue': payload_type,
                        'DataType': 'String'
                    }                 
            }     

class Sqs(object):

    def __init__(self, context, queue_prefix):        
        self.__context = context        
        self.__is_all_under_load = False
        self.__client = boto3.client('sqs',region_name=os.environ[c.ENV_REGION], api_version='2012-11-05')
        self.__queue_prefix = queue_prefix            
        self.__queue_url = context[c.KEY_SQS_QUEUE_URL] if c.KEY_SQS_QUEUE_URL in context and context[c.KEY_SQS_QUEUE_URL] else None
        self.__max_message_size = c.MAXIMUM_MESSAGE_SIZE_IN_BYTES  
        print "Queue prefix", queue_prefix   

    @property
    def queue_url(self):
        return self.__queue_url

    @property
    def is_all_under_load(self):
        if self.__queue_url is None:
            self.set_queue_url(True)
        return self.__is_all_under_load
            

    def drop(self, receiptid, body, attempts):
        print "Dropping message after {} attempts that had a message body of \n{}".format(attempts, body)
        self.__context[c.KEY_THREAD_POOL].add(retry.try_with_backoff, self.__context, self.__client.delete_message, QueueUrl=self.__queue_url, ReceiptHandle=receiptid)

    def delete_message_batch(self, metrics):
        start = time.time()      
        msgs_to_delete = set(self.__context[c.KEY_SUCCEEDED_MSG_IDS])  
        print "Total number of messages to delete are {}".format(len(msgs_to_delete))
        threadpool = self.__context[c.KEY_THREAD_POOL]        
        url = self.__queue_url    
        msgs_to_delete = util.split(msgs_to_delete, 10, self.__delete_wrapper)
        for msg_set in msgs_to_delete:            
            threadpool.add(retry.try_with_backoff, self.__context, self.__client.delete_message_batch, QueueUrl=url, Entries=msg_set)
        threadpool.wait()    
        return int(time.time() - start)

    def __delete_wrapper(self, item):
        item = item.split(self.__context[c.KEY_SEPERATOR_CSV]) 
        return ({
                'Id': item[0],
                'ReceiptHandle': item[1]
            })


    def read_queue(self):    
        timeout = self.__context[c.KEY_MAX_LAMBDA_TIME] + 30
        response = retry.try_with_backoff(self.__context, self.__client.receive_message, \
                            QueueUrl=self.__queue_url, \
                            AttributeNames=['ApproximateReceiveCount'], \
                            MessageAttributeNames=['All'], \
                            MaxNumberOfMessages=10, \
                            VisibilityTimeout=timeout, \
                            WaitTimeSeconds=5 #enables log polling http://docs.aws.amazon.com/AWSSimpleQueueService/latest/SQSDeveloperGuide/sqs-long-polling.html
                            )
        if response is None or 'Messages' not in response:
            return []
        return response['Messages'] 

    def get_queue_attributes(self, queue_url=None):
        if queue_url is None:
            queue_url = self.__queue_url 
        return retry.try_with_backoff(self.__context, self.__client.get_queue_attributes, QueueUrl=queue_url,AttributeNames=['ApproximateNumberOfMessagesNotVisible', 'ApproximateNumberOfMessages'])

    def send_message(self, sensitivity_type, data, compression_mode, payload_type):      
        params = self.__create_message(sensitivity_type, data, compression_mode, payload_type)
        params[c.SQS_PARAM_QUEUE_URL] = self.__queue_url     
        response=retry.try_with_backoff(self.__context, self.__client.send_message, **params)
        return response    

    #the entire batch is limited to 256KB, send_message is better
    def send_message_batch(self, sensitivity_type, data, compression_mode, payload_type):           
        params = dict({})     
        params[c.SQS_PARAM_QUEUE_URL] = self.__queue_url  
        params["Entries"] = []
        for message in data:    
            batch_item = self.__create_message(sensitivity_type, message, compression_mode, payload_type)
            batch_item["Id"] = uuid.uuid1().hex
            params["Entries"].append(batch_item)
        response=retry.try_with_backoff(self.__context, self.__client.send_message_batch, **params)
        return response    

    def get_queues(self):
        response = retry.try_with_backoff(self.__context, self.__client.list_queues,
               QueueNamePrefix=self.__queue_prefix
        )
        if 'QueueUrls' not in response:
            print "There are no queues found using queue prefix '{}'".format(self.__queue_prefix)
            return []
        return response['QueueUrls']

    def set_queue_url(self, lowest_load_queue):
        queues = self.get_queues()
        message_count = sys.maxint if lowest_load_queue else 0
        idx_to_use = 0
        is_all_under_load = True        
        idx = 0                
        for queue_url in queues:            
            response = self.get_queue_attributes(queue_url)            
            messages_to_process = int(response['Attributes']['ApproximateNumberOfMessages'])
            inflight_messages = int(response['Attributes']['ApproximateNumberOfMessagesNotVisible']) 
            if lowest_load_queue:
                #find the least stressed queue based on number of messages due to be processed
                if messages_to_process < message_count:
                    message_count = messages_to_process
                    idx_to_use = idx
            else:
                #find the most stressed queue based on number of messages due to be processed that is not above 50% inflight messages                                
                if messages_to_process > message_count and inflight_messages < self.__context[c.KEY_FIFO_GROWTH_TRIGGER]:
                    message_count = messages_to_process
                    idx_to_use = idx
            print ("Queue '{}' has {} in-flight messages and has {} messages to process.").format(queue_url, inflight_messages, messages_to_process)          
            
            is_all_under_load &= (inflight_messages > self.__context[c.KEY_FIFO_GROWTH_TRIGGER])
            idx+=1                
        
        self.__is_all_under_load = is_all_under_load
        
        self.__queue_url = queues[idx_to_use]
    
    def add_fifo_queue(self, prefix):
        range_length = 80 - len(prefix) - 6         
        key = ''.join(random.choice('0123456789ABCDEFGHIJKLMNOPQRSTUVWXZY') for i in range(range_length))               
        name = "{}{}{}.fifo".format(prefix, c.FILENAME_SEP, key)        
        print "Adding new SQS FIFO queue named '{}'".format(name)         
        response = self.__client.create_queue(
            QueueName=name,
            Attributes={
                'FifoQueue': 'true',
                'VisibilityTimeout': '300'
            }            
        )
        return name

    def get_max_message_size(self):
        response=retry.try_with_backoff(self.__context, self.__client.get_queue_attributes, QueueUrl=self.__queue_url,AttributeNames=['All'])
        if response is None:
            return []
        return response['Attributes']['MaximumMessageSize']

    def delete_all_queues(self, prefix):
        queues = self.get_queues()
        for url in queues:
            if prefix in url:
                print "Deleting queue '{}'".format(url)
                retry.try_with_backoff(self.__context, self.__client.delete_queue, QueueUrl=url)

    def __create_message(self, sensitivity_type, data, compression_mode, payload_type ):
        params = dict({})     
        params[c.SQS_PARAM_MESSAGE_GROUP_ID]= uuid.uuid1().hex          
        params[c.SQS_PARAM_MESSAGE_DEPULICATIONID]= uuid.uuid1().hex
        params[c.SQS_PARAM_DELAY_SECONDS]= 0
        params[c.SQS_PARAM_MESSAGE_ATTRIBUTES]= message_attributes(sensitivity_type.identifier, compression_mode.identifier, payload_type.identifier)
        
        compression_mode.add_message_payload(params, data) 
        return params