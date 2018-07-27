from partitioner import Partitioner
from StringIO import StringIO
from compression import Compress, NoCompression, CompressionClassFactory
from sensitivity import SensitivityClassFactory
from payload import CSV, JSON, PayloadClassFactory
from metric_schema import Required, Type, Order
from collections import OrderedDict
import time
import metric_constant as c
import datetime
import ctypes
import util
import sqs
import logging
import metric_schema


class Aggregator(object):
    """
        Aggregate events of multiple different SQS messages into S3 key lists.
    """
    def __init__(self, context, partition_set):                  
        self.__aggregation_sets = partition_set 
        self.__aggregation_sets[c.KEY_TABLES] = {} 
        self.__partitioner = Partitioner(context[c.KEY_PARTITIONS], context[c.KEY_SEPERATOR_PARTITION])
        self.__context = context
        self.__info = {}
        self.__info[c.INFO_TOTAL_BYTES] = 0
        self.__info[c.INFO_TOTAL_ROWS] = 0
        self.__info[c.INFO_TOTAL_MESSAGES] = 0        
        self.__logger = logging.getLogger()
        self.__logger.setLevel(logging.ERROR)     

    @property
    def bytes_uncompressed(self):        
        return self.__info[c.INFO_TOTAL_BYTES]

    @property
    def rows(self):
        return self.__info[c.INFO_TOTAL_ROWS] 

    @property
    def messages(self):
        return self.__info[c.INFO_TOTAL_MESSAGES] 

    @property
    def info(self):                
        return self.__info
          
    def append_default_metrics_and_partition(self, messages):                
        length = len(messages)
        util.debug_print(("Processing {} messages.").format(length))
        self.increment(self.__info, c.INFO_TOTAL_MESSAGES, length)        
        for x in xrange(0,length):
           message = messages[x]   
           self.process_message(message)

    def process_message(self, message):        
        util.debug_print(("Processing message:\n{}").format(message))                 
        compression_mode = CompressionClassFactory.instance(message[c.SQS_PARAM_MESSAGE_ATTRIBUTES][c.SQS_PARAM_COMPRESSION_TYPE]['StringValue'])        
        body = compression_mode.extract_message_body(message)        
        attempts = int(message['Attributes']['ApproximateReceiveCount'])               
        sensitivity_type = SensitivityClassFactory.instance(message[c.SQS_PARAM_MESSAGE_ATTRIBUTES][c.SQS_PARAM_SENSITIVITY_TYPE]['StringValue'])                
        payload_type = PayloadClassFactory.instance( self.__context, message[c.SQS_PARAM_MESSAGE_ATTRIBUTES][c.SQS_PARAM_PAYLOAD_TYPE]['StringValue'], compression_mode, sensitivity_type)        
        
        msg_token = "{}{}{}".format(message['MessageId'], self.__context[c.KEY_SEPERATOR_CSV], message['ReceiptHandle'])
        if attempts > self.__context[c.KEY_MAX_MESSAGE_RETRY]:
            self.__logger.error("The message with message Id {} has been processed {} times.".format(msg_token, attempts))
        self.increment(self.__info, c.INFO_TOTAL_BYTES, len(body))      
        
        payload_type.to_partitions(msg_token, body, self.partition, sensitivity_type, self.__partitioner.partitions)                    

    def partition(self, token, row, sensitivity_type):        
        #schema hash                                   
        schema_hash = hash(str(row.keys()))  
        
        uuid_key = "{}{}{}".format(row[metric_schema.UUID.id], row[metric_schema.EVENT.id], row[metric_schema.SERVER_TIMESTAMP.id])               
        #create the key here as the partition my remove attributes if the attribute is created as a partition        
        tablename, partition = self.__partitioner.extract(schema_hash, row, sensitivity_type)         
        columns, row = self.order_and_map_to_long_name(row)         
        
        if partition is None:
            self.__logger.error("Dropping metric\n{}".format(row))
            return
                
        if partition not in self.__aggregation_sets:
            #need to use a immutable object as required by fastparquet for hashing
            self.__aggregation_sets[partition] = dict({})  

        if tablename not in self.__aggregation_sets[c.KEY_TABLES]:
            self.__aggregation_sets[c.KEY_TABLES][tablename]=tablename          
        
        partition_dict = self.__aggregation_sets[partition]             
        if schema_hash not in partition_dict:                                                  
            partition_dict[schema_hash] = {}
            partition_dict[schema_hash][c.KEY_SET] = {}
            partition_dict[schema_hash][c.KEY_SET_COLUMNS] = columns            
        partition_dict[schema_hash][c.KEY_SET][uuid_key] = row            
        
        self.register_processed_message(partition_dict[schema_hash], token)         

    def register_processed_message(self, schema_dict, msg_token):        
        #track which messages have been processed
        if c.KEY_MSG_IDS not in schema_dict:
            schema_dict[c.KEY_MSG_IDS], schema_dict[c.KEY_APPENDER] = self.get_new_list_append_handler()                     

        if msg_token not in schema_dict[c.KEY_MSG_IDS]:
            schema_dict[c.KEY_APPENDER](msg_token)                                               

    def get_new_list_append_handler(self):
        list = []
        append = list.append
        return list, append

    def increment(self, dict, key, value):
        if key not in dict:
            dict[key] = value
        dict[key] += value

    def order_and_map_to_long_name(self, row):        
        orderer = Order()
        ordered_columns = orderer.order_columns(row)        
        ordered_dict = OrderedDict()
        ordered_columns_long_name = []
        for field in ordered_columns:                        
            if field not in row:
                continue
            value = row[field]
            if field in metric_schema.DICTIONARY and field in row: 
                name = metric_schema.DICTIONARY[field].long_name 
                ordered_dict[name] = value         
                ordered_columns_long_name.append(name)
            else:
                ordered_dict[field] = value
                ordered_columns_long_name.append(field)
        
        return ordered_columns_long_name, ordered_dict        
