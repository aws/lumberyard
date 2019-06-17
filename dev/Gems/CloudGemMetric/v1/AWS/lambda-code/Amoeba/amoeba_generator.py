from s3 import S3
from keyparts import KeyParts
from botocore.exceptions import ClientError
from dynamodb import DynamoDb
import os
import metric_constant as c
import parquet_writer as writer
import parquet_reader as reader
import s3fs
import util
import mem_util as mutil
import time
import pandas as pd
import metric_schema as m_schema
import datetime
import gc
import json
import sys
import imp
from sqs import Sqs

debug_print = util.debug_print

def bucket():
    '''Returns the target S3 bucket.'''
    if not hasattr(bucket, 'id'):
        bucket.id = os.environ[c.ENV_S3_STORAGE]
    return bucket.id   

def s3():
    '''Returns a S3 client with proper configuration.'''
    if not hasattr(s3, 'client'):
        s3.client = S3(context(), bucket())   
    return s3.client  

def context(event = None):
    '''Returns an initialized context.'''    
    if not hasattr(context, 'client'):         
        debug_print("Initializing the context for the first time.")
        context.client = event.get("context", {})                
        DynamoDb(context.client)                 
    return context.client 

def ingest(event, lambdacontext):        
    debug_print("Initial memory size: {} bytes and {}%".format(mutil.get_memory_object(), mutil.get_memory_usage()))        
    amoeba = Amoeba(event, lambdacontext)
    amoeba.ingest()    
    del amoeba
    gc.collect()
    debug_print("Initial memory size: {} bytes and {}%".format(mutil.get_memory_object(), mutil.get_memory_usage()))

class Amoeba(object):

    def __init__(self, event, lambdacontext):
        self.context = context(event)  
        self.starttime = time.time()         
        self.s3 = s3()
        self.bucket = bucket()        
        self.files = event.get("files", None)   
        self.directory = event.get("directory", None)        
        self.context[c.KEY_LAMBDA_FUNCTION] = lambdacontext.function_name if hasattr(lambdacontext, 'function_name') else None       
        self.context[c.KEY_START_TIME] = self.starttime                        

    def ingest(self):                    
        debug_print("Ingesting directory {}".format(self.directory))               
        debug_print("Ingesting the files \n{}".format(self.files))                    
        is_lambda =  self.context[c.KEY_LAMBDA_FUNCTION] is not None
        timeout = self.__calculate_aggregate_window_timeout(self.context[c.KEY_MAX_LAMBDA_TIME])
        target_excretion_size = self.context[c.KEY_TARGET_AGGREGATION_FILE_SIZE_IN_MB]
        compression_ratio = self.context[c.KEY_CSV_PARQUET_COMPRESSION_RATIO]
        sep = self.context[c.KEY_SEPERATOR_PARTITION]
        memory_trigger = self.context[c.KEY_AMOEBA_MEMORY_FLUSH_TRIGGER]             
        memory_used = mutil.get_memory_usage()             
        main_filename, main_file_data, main_file_size_mb = self.__get_main_aggregate_file(self.directory, sep, target_excretion_size)        
        main_file_data = self.__append(None, main_file_data)           
        keys_ingested = []
        for file in self.files:
            debug_print("\tProcessing file {}".format(file))
            key_parts = KeyParts(file, sep)
            duration = datetime.datetime.utcnow() - key_parts.filename_timestamp            
            if duration.total_seconds() < 300:
                debug_print("The file '{}' is {}s old.  It is too new and will be processed later to allow for S3 propagation.".format(file, duration.total_seconds()))
                continue
            keys_ingested.append(file)            
            data = self.__open(file, main_file_data)
            if data is None:
                continue            
            size_in_megabytes = self.__size(file)            
            main_file_data = self.__append(main_file_data, data) 
            del data
            gc.collect()
            current_dataframe_size = sys.getsizeof(main_file_data)        
            #break conditions
            #1. Memory limit exceeded
            #2. Time window exceeded
            #3. Target excretion size hit
            main_file_size_mb += size_in_megabytes
            memory_used = mutil.get_memory_usage()           
            debug_print("\t\tSize on S3: {}MB Size of new dataset: {}bytes Estimated Compression Ratio: {} Memory Used: {}% Project Compression Size {}MB  Target Excretion Size {}MB".format(size_in_megabytes, current_dataframe_size, compression_ratio, memory_used, main_file_size_mb, target_excretion_size))
            if util.elapsed(self.context) > timeout or memory_used > memory_trigger or main_file_size_mb > target_excretion_size :
                print "Elapsed", util.elapsed(self.context), "Start:", self.starttime, "Timeout:", timeout, "Has timed out:", util.elapsed(self.context) > timeout, "Mem Used %:", memory_used, "Max Memory %:", memory_trigger
                break                
        
        #only save the files if we have a reasonable amount of time remaining before the lambda timeout.
        debug_print("Time remaining: {}s".format(util.time_remaining(self.context)))    
        debug_print("There were {} keys ingested.  The keys ingested are: \n {}".format(len(keys_ingested), keys_ingested))
        if len(keys_ingested)>0 and util.time_remaining(self.context) > c.SAVE_WINDOW_IN_SECONDS and not main_file_data.empty:            
            main_file_data = self.__convert_to_submission_df(main_file_data)
            gc.collect()
            self.__excret(self.directory, main_filename, main_file_data, sep)            
            self.__delete_keys(keys_ingested)
        elif util.time_remaining(self.context) <= c.SAVE_WINDOW_IN_SECONDS:            
            print "Time has run out!  We have less than {} seconds remaining before this lambda times out.  Abandoning the S3 commit to avoid file corruption.".format(c.SAVE_WINDOW_IN_SECONDS)
            print "Aggregation window (Max Lambda Execution Time * {}): {} seconds".format(c.RATIO_OF_MAX_LAMBDA_TIME, timeout) 
            print "S3 Save window: {} seconds".format(c.SAVE_WINDOW_IN_SECONDS) 
            print "Lambda time remaining: {} seconds".format(util.time_remaining(self.context))                        
           
        remaining_files = list(set(self.files) - set(keys_ingested))
        if len(remaining_files) > 0:        
            debug_print("Re-adding the {} paths to SQS to attempt again. The paths are \n{}".format(len(remaining_files), remaining_files))               
            self.__add_to_sqs(remaining_files)        
        print "I've consumed everything I can in bucket '{}'".format(self.directory)
        return

    def __size(self, key):        
        try:        
            return self.s3.size_in_megabytes(key)                    
        except ClientError as e:        
            if str(e.response['Error']['Code']) == '404':                
                return None
            else:
                print "Error: ", e.response['Error']['Code'], key
                raise e        
        return None

    def __convert_to_submission_df(self, data):        
        values = data.values        
        columns = list(data.columns.values)
        return pd.DataFrame(values, columns=columns)

    def __open(self, key, main_file_data):        
        try:
            data = reader.read(s3fs.S3FileSystem(), self.bucket, key)            
            return data           
        except ClientError as e:        
            print e.response['Error']['Code'], "key=>", key                                                
            #handle corrupt files, this can happen if a write did not finish correctly                
            if e.message == "Seek before start of file":
                self.__move_corrupt_file(self.s3, key)
            elif e.response['Error']['Code'] == 'NoSuchKey':                
                print '{}: for key {}'.format(e.response['Error']['Code'], key)
            else:
                util.logger.error(e) 
        except Exception as unexpected:
            util.logger.error(unexpected)
        return None

    def __add_to_sqs(self, files):
        prefix = util.get_stack_name_from_arn(os.environ[c.ENV_DEPLOYMENT_STACK_ARN])    
        sqs = Sqs(self.context, "{0}{1}".format(prefix, c.KEY_SQS_AMOEBA_SUFFIX))
        sqs.set_queue_url(lowest_load_queue=False)  
        sqs.send_generic_message(json.dumps({ "paths": files }))

    def __append(self, master, data):        
        if master is None:
            if data is None:
                return pd.DataFrame()                
            return data        
        start = time.time()          
        master = pd.concat([master, data])        
        return master 

    def __get_main_aggregate_file(self, directory, seperator, max_file_size_in_mb):    
        i = 0    
        debug_print("Getting main file for {}".format(directory))            
        while True:        
            filename = "combined_file_{}.parquet".format(i)
            key = "{0}{1}{2}".format(directory, seperator, filename)            
            try:                        
                file_size_in_mb = self.s3.size_in_megabytes(key)
                debug_print("Trying {0}, Size {1}MB".format(key, file_size_in_mb))                
                if file_size_in_mb < max_file_size_in_mb: 
                    data = reader.read(s3fs.S3FileSystem(), self.bucket, key)   
                    return filename, data, file_size_in_mb
            except Exception as e:                                                 
                print e            
                #handle corrupt files, this can happen if a write did not finish correctly                
                if e.message == "Seek before start of file" or "invalid code lengths set" in e.message:                                
                    self.__move_corrupt_file(s3, key)                                 
                return filename, pd.DataFrame(), 0      
            i += 1

    def __move_corrupt_file(self, key):        
        corrupt_path = "corrupt_data_files{}".format(key)
        print "Moving corrupt file '{}' to '{}' for further analysis.".format(key, corrupt_path)                      
        body = self.s3.get_object(util.get_path_without_leading_seperator(key))["Body"].read()
        self.s3.put_object(corrupt_path,body)  
        self.s3.delete(util.get_path_without_leading_seperator(key))  

    def __excret(self, key, filename, data, sep):            
        output_path = "{}{}{}".format(key,sep,filename)
        print "\t\tSaving ingested file to '{}{}'".format(self.bucket, output_path)            
        writer.write(self.bucket, output_path, data, sep, m_schema.object_encoding(data.columns.values)) 
        
    def __delete_keys(self, keys):
        debug_print("Deleting {}".format(keys))
        self.s3.delete(keys)

    def __calculate_aggregate_window_timeout(self, maximum_lambda_duration):     
        maximum_aggregation_period = maximum_lambda_duration * c.RATIO_OF_MAX_LAMBDA_TIME    
        return maximum_aggregation_period

    

