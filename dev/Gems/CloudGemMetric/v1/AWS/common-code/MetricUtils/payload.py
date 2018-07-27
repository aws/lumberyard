from io import BytesIO
from abc import ABCMeta, abstractmethod
from StringIO import StringIO
from datetime import datetime, timedelta
import sqs
import pandas as pd
import metric_constant as c
import os
import gzip
import zlib
import base64
import re
import time
import json
import compression
import errors
import metric_schema as schema
import metric_error_code as code
import collections
import timeit
import enum_type
import uuid

PAYLOAD_TYPE = enum_type.create(CSV="CSV", JSON="JSON")

class PayloadClassFactory():
    @staticmethod
    def instance(context, name, compression_mode, sensitivity_type, source_IP = None):
        #done this way for performance
        if name.lower() == PAYLOAD_TYPE.CSV.lower():
            return CSV(context, compression_mode, sensitivity_type, source_IP)
        if name.lower() == PAYLOAD_TYPE.JSON.lower():
            return JSON(context, compression_mode, sensitivity_type, source_IP)


class AbstractPayload:
    __metaclass__ = ABCMeta

    """
        Handle the different supported payload types
    """
    def __init__(self, context, compression_mode, sensitivity_type, source_IP):
        self.__context = context 
        self.__compression_mode = compression_mode
        self.__sensitivity_type = sensitivity_type
        self.__source_ip = source_IP
        self.__schema_order = schema.Order()
        self.__longitude = 0.0
        self.__latitude = 0.0
        #TODO: Populate country of origin based on requestors source IP address.
        self.__country_of_origin = "Unknown"

    @property
    def identifier(self):        
        return self.__class__.__name__

    @property
    def context(self):
        return self.__context

    @property
    def compression_mode(self):
        return self.__compression_mode

    @property
    def sensitivity_type(self):
        return self.__sensitivity_type

    @property
    def source_ip(self):
        return self.__source_ip

    @property
    def schema_order(self):
        return self.__schema_order

    @property
    def longitude(self):
        return self.__longitude

    @property
    def latitude(self):
        return self.__latitude

    @property
    def country_of_origin(self):
        return self.__country_of_origin

    @abstractmethod
    def extract(self, data):
        pass

    @abstractmethod
    def chunk(self, data):
        pass

    @abstractmethod
    def append_server_metrics(self, data):
        pass

    @abstractmethod
    def to_partitions(self, token, data, func, sensitivity_type):
        pass

    @abstractmethod
    def validate_values(self, *params):
        pass 

    def validate(self, columns):        
        required_fields_ordered = self.schema_order.required_fields        
        for field in required_fields_ordered:
            if field.id not in columns:
                raise errors.ClientError("[{}] The metric is missing the attribute '{}' in columns {}".format(code.Error.missing_attribute(), field.id, columns))
        
        for field in columns:
            if not field.islower():                
                raise errors.ClientError("[{}] The metric attribute '{}' is not lowercase.  All attributes should be lowercase.  The columns were '{}'".format(code.Error.is_not_lower(), field, columns))

        return 

    def to_string(self, data):
        pass

    def header(self, data):
        pass

    def current_unix_datetime(self):        
        return time.time()


class CSV(AbstractPayload):
    def __init__(self, context, compression_mode, sensitivity_type, source_IP):
        AbstractPayload.__init__(self, context, compression_mode, sensitivity_type, source_IP)  

    def extract(self, data): 
        pattern = r'''\\n|\n'''        
        return re.split(pattern, data)

    def to_string(self, data):         
        return self.context[c.KEY_SEPERATOR_CSV].join(data)

    def terminate_line(self, data):        
        return data + c.NEW_LINE

    def to_partitions(self, token, data, func, sensitivity_type, partitions):        
        data = pd.read_csv(StringIO(data),sep=self.context[c.KEY_SEPERATOR_CSV], encoding="utf-8")          
        #iterator the rows in the data set to partition the data correctly
        #partition the data            
        #ie.  this message belongs in s3 path /2017/09/27/23/shotFired                            
        for row in data.itertuples(index=True): 
            row = row.__dict__
            del row['Index']              
            func(token, row, sensitivity_type)

        del data

    def header(self, data):
        return data[0]

    # Append server metrics to the metric payload
    def append_server_metrics_to_header(self, header):
        #append region information based on ip address        
        #append timestamp  
        header.append(schema.Required.Server.uuid().id)
        header.append(schema.Required.Server.server_timestamp_utc().id)
        header.append(schema.Required.Server.longtitude().id)  
        header.append(schema.Required.Server.latitude().id)  
        header.append(schema.Required.Server.country().id)  

    # Append server metrics to the metric payload
    def append_server_metrics(self, metric):        
        metric.append(uuid.uuid1().hex)
        #must be a string for String IO concatenation
        metric.append(str(self.current_unix_datetime()))
        metric.append(str(self.longitude) if self.context[c.KEY_SAVE_GLOBAL_COORDINATES] else "0.0")
        metric.append(str(self.latitude) if self.context[c.KEY_SAVE_GLOBAL_COORDINATES] else "0.0")
        metric.append(str(self.country_of_origin))

    def validate_values(self, header, metric):
        
        required_fields_ordered = self.schema_order.required_fields        
        for field in required_fields_ordered:
            idx = header.index(field.id)            
            value = metric[idx] 
            if value is None or len(str(value)) == 0:
                print header
                print metric
                raise errors.ClientError("[{}] The metric attribute '{}' is null or empty.  The value was '{}'. Required fields can not be null or empty.".format(code.Error.is_not_lower(), field.id, value))

    def chunk(self, data):
        metrics = self.extract(data)        
        sep = self.context[c.KEY_SEPERATOR_CSV]
        messages = []
        metrics_length = len(metrics)    
        if metrics_length == 0:
            return messages

        message_chunk = StringIO()            
        header = metrics[0].split(sep)  
        self.append_server_metrics_to_header(header)        
        header = [item.lower() for item in header]
        self.validate(header) 
        header_list = header  
        header = self.to_string(header)
        header_size = len(header)

        message_chunk.write(self.terminate_line(header))
        if os.environ[c.ENV_VERBOSE]== "True":
            print "The message header is {} bytes".format(header_size)
        
        total_metrics = 0
        total_metrics_per_chunk = 0    
        for metric in metrics[1:]:                      
            if len(metric) == 0:            
                continue
            metric = metric.split(sep)
            self.append_server_metrics(metric)  
            self.validate_values(header_list, metric)            
            metric = self.terminate_line(self.to_string(metric))            
            metric_size = self.compression_mode.size_of(self.compression_mode.compress(metric))        
            if metric_size + header_size > c.MAXIMUM_MESSAGE_SIZE_IN_BYTES:
                raise errors.ClientError("[{}] The maximum of {} compressed bytes have been exceeded. The metric contains {} bytes.".format(code.Error.missing_attribute(), c.MAXIMUM_MESSAGE_SIZE_IN_BYTES, metric_size + header_size))
            #do not pack more into the message than the SQS payload limit of 256KB.
            
            message = message_chunk.getvalue()        
            chunk = self.compression_mode.compress(message)            
            message_size = self.compression_mode.size_of(chunk) + sqs.message_overhead_size(self.sensitivity_type, self.compression_mode, self)   
            
            if message_size + metric_size >= c.MAXIMUM_MESSAGE_SIZE_IN_BYTES:                        
                if os.environ[c.ENV_VERBOSE]== "True":
                    print "message size(bytes): ", message_size, " total metrics in chunk: ", total_metrics_per_chunk                   
                messages.append(chunk)            
                message_chunk = StringIO()
                message_chunk.write(self.terminate_line(header))
                total_metrics_per_chunk = 0
            message_chunk.write(self.terminate_line(metric))        
            total_metrics += 1
            total_metrics_per_chunk += 1
        message = message_chunk.getvalue()
        if os.environ[c.ENV_VERBOSE]== "True":
            print "message size(bytes): ", message_size, " total metrics in chunk: ", total_metrics_per_chunk   
        message_chunk.close()
        if len(message) > 0:        
            messages.append(self.compression_mode.compress(message)) 
    
        return messages, total_metrics 
   
class JSON(AbstractPayload):
    def __init__(self, context, compression_mode, sensitivity_type, source_IP):
        AbstractPayload.__init__(self, context, compression_mode, sensitivity_type, source_IP)        

    def extract(self, data):        
        return json.loads(data)

    def to_partitions(self, token, data, func, sensitivity_type, partitions):
        data = self.extract(data)  
        #iterator the rows in the data set to partition the data correctly
        #partition the data            
        #ie.  this message belongs in s3 path table=<event>/2017/09/27/23/                     
        #order of the dict keys matters, if it is in the wrong order it will not match the database schema
        for row in data:                     
            func(token, row, sensitivity_type)

    def to_string(self, data):
        return json.dumps(data)

    # Append server metrics to the metric payload
    def append_server_metrics(self, metric):            
        metric[schema.Required.Server.uuid().id]= uuid.uuid1().hex 
        metric[schema.Required.Server.server_timestamp_utc().id]= self.current_unix_datetime()
        #it is against the law in some countries to save personally identifiable information in some countries like Europe zone.   Enough lat/longs and you can track a person. 
        metric[schema.Required.Server.longtitude().id]= self.longitude if self.context[c.KEY_SAVE_GLOBAL_COORDINATES] else 0.0
        metric[schema.Required.Server.latitude().id]= self.latitude if self.context[c.KEY_SAVE_GLOBAL_COORDINATES] else 0.0
        metric[schema.Required.Server.country().id]= self.country_of_origin

    def validate_values(self, dict):        
        required_fields_ordered = self.schema_order.required_fields        
        for field in required_fields_ordered:
            value = dict[field.id]
            if value is None or len(str(value)) == 0:
                raise errors.ClientError("[{}] The metric attribute '{}' is null or empty.  Required fields can not be null or empty.".format(code.Error.is_not_lower(), field.id))
  
    def chunk(self, data):
        metrics = self.extract(data)
        messages = []
        metrics_length = len(metrics)    
        if metrics_length == 0:
            return messages

        message_chunk = []
        total_metrics = 0
        total_metrics_per_chunk = 0    
        for metric in metrics:                      
            if len(metric) == 0:            
                continue
            
            self.append_server_metrics(metric) 
            self.validate(metric.keys())           
            self.validate_values(metric)
            metric_size = self.compression_mode.size_of(self.compression_mode.compress(self.to_string(metric)))+2
            if metric_size > c.MAXIMUM_MESSAGE_SIZE_IN_BYTES:
                raise errors.ClientError("[ExceededMaximumMetricCapacity] The maximum of {} compressed bytes have been exceeded. The metric contains {} bytes.".format(c.MAXIMUM_MESSAGE_SIZE_IN_BYTES, metric_size))
            #do not pack more into the message than the SQS payload limit of 256KB.
            message = self.to_string(message_chunk)
            chunk = self.compression_mode.compress(message)  
            message_size = self.compression_mode.size_of(chunk) + sqs.message_overhead_size(self.sensitivity_type, self.compression_mode, self)   
            if message_size + metric_size >= c.MAXIMUM_MESSAGE_SIZE_IN_BYTES:                        
                if os.environ[c.ENV_VERBOSE]== "True":
                    print "message size(bytes): ", message_size, " total metrics in chunk: ", total_metrics_per_chunk   
                messages.append(chunk)
                message_chunk = []                
                total_metrics_per_chunk = 0
            message_chunk.append(metric)  
            total_metrics += 1
            total_metrics_per_chunk += 1
        message = message_chunk
        if os.environ[c.ENV_VERBOSE]== "True":
            print "message size(bytes): ", message_size, " total metrics in chunk: ", total_metrics_per_chunk   
        
        if len(message_chunk) > 0:        
            messages.append(self.compression_mode.compress(self.to_string(message_chunk))) 
        
        return messages, total_metrics 




