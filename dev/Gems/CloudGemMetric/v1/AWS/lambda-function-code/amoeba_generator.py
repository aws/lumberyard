from s3crawler import Crawler
from s3 import S3
from keyparts import KeyParts
from botocore.exceptions import ClientError
import os
import metric_constant as c
import parquet_writer as writer
import parquet_reader as reader
import s3fs
import uuid
import util
import mem_util as mutil
import time
import pandas as pd
import metric_schema as m_schema
import datetime
import gc

def ingest(event, lambdacontext):           
    starttime = time.time()
    gc.collect()
    root = event.get("root", None)   
    print "Initial memory size:",  mutil.get_memory_object()    
    print "Started amoeba with root {}".format(root)     
    context = event.get("context", {})    
    context[c.KEY_LAMBDA_FUNCTION] = lambdacontext.function_name if hasattr(lambdacontext, 'function_name') else None       
    context[c.KEY_START_TIME] = starttime
    is_lambda =  context[c.KEY_LAMBDA_FUNCTION] is not None
    bucket = os.environ[c.ENV_S3_STORAGE]
    crawler = Crawler(context, bucket)
    roots = crawler.crawl_from_relative(root)    
    s3_fs = s3fs.S3FileSystem() 
    s3 = S3(context, bucket)    
    timeout = calculate_aggregate_window_timeout(context[c.KEY_MAX_LAMBDA_TIME] )
    target_excretion_size = context[c.KEY_TARGET_AGGREGATION_FILE_SIZE_IN_MB]
    compression_ratio = context[c.KEY_CSV_PARQUET_COMPRESSION_RATIO]
    sep = context[c.KEY_SEPERATOR_PARTITION]
    memory_used = mutil.get_memory_usage() 
    projected_compressed_file_size_in_mb = 0
    print "Hunting for {} seconds in bucket '{}'".format(timeout, bucket)
    for path in roots: 
        #The GLUE Crawler does not work well when a single key in S3 contains varying data schemas.
        files = roots[path]
        if len(files) == 1:
             continue        
        debug_print( "\t\tIngesting path {}".format(path))        
        df = {}    
        keys_ingested = []    
        data = None
        
        for file in files:
            debug_print("\t\t\t{}".format(file))                
            key = "{}/{}".format(path, file)      
            try:
                size_in_megabytes = s3.size_in_megabytes(key)                
            except ClientError as e:                  
                if str(e.response['Error']['Code']) == '404':                    
                    continue
                else:
                    print "Error: ", e.response['Error']['Code'], key                
                    raise e
            
            if size_in_megabytes > target_excretion_size:
                debug_print("Skipping file '{}'.  It has reached the targetted file size".format(key))
                continue                 
            size_in_bytes = size_in_megabytes*1024*1024
            try:
                data = reader.read(s3_fs, bucket, key)                     
                keys_ingested.append(key)
            except ClientError as e:                                
                print e.response['Error']['Code'], "key=>", key                                                
                #handle corrupt files, this can happen if a write did not finish correctly                
                if e.message == "Seek before start of file":
                    print "Deleting corrupt file %s", key
                    s3.delete([key])
                elif e.response['Error']['Code'] == 'NoSuchKey':                
                    print '{}: for key {}'.format(e.response['Error']['Code'], key)
                else:
                    util.logger.error(e)                  
                continue
            for row in data.itertuples(index=True): 
                row = row.__dict__                
                del row['Index']
                key_parts = KeyParts(key, sep)
                uuid_key = "{}{}{}".format(row[c.PRIMARY_KEY], key_parts.event, row[c.TERTIARY_KEY])
                df_size = len(row)

                debug_print("\t\t\tSize on S3 in MB: {} Size as Dataframe: {} Ratio: {}".format(size_in_megabytes, df_size, compression_ratio))
                    
                #a dictionary is the fastest way to create a unique set.                                                        
                if uuid_key in df:
                    debug_print("\t\t\tFound duplication in key '{}'.  Keeping the first occurrence.".format(key))
                else:
                    df[uuid_key] = row
                
                current_dataframe_size = len(df)
                #break conditions
                #1. Memory limit exceeded
                #2. Time window exceeded
                #3. Target excretion size hit
                projected_compressed_file_size_in_mb = (compression_ratio * current_dataframe_size) / 1048576.0
                memory_used = mutil.get_memory_usage() 
                debug_print("\t\t\t{} seconds have elapsed.  {} kilobytes of memory have been used. The projected compressed file size is {} MB.  We are targetting an excretion file size of {} MB.".format(util.elapsed(context), memory_used/1024, projected_compressed_file_size_in_mb, target_excretion_size))                
                if util.elapsed(context) > timeout or memory_used > context[c.KEY_AMOEBA_MEMORY_FLUSH_TRIGGER] or projected_compressed_file_size_in_mb > target_excretion_size :
                    break                            
            if util.elapsed(context) > timeout or memory_used > context[c.KEY_AMOEBA_MEMORY_FLUSH_TRIGGER] or projected_compressed_file_size_in_mb > target_excretion_size :
                print "Elapsed", util.elapsed(context), "Start:", context[c.KEY_START_TIME], "Timeout:", timeout, "Has timed out:", util.elapsed(context) > timeout, "Mem Used:", memory_used, "Max Memory %:", context[c.KEY_AMOEBA_MEMORY_FLUSH_TRIGGER]
                break   
        if len(keys_ingested)>0 and util.time_remaining(context) > 45:     
            values = df.values()
            #since the schema hash of the event columns are all the same we can infer the dict[0].keys has the same column headers as the rest            
            columns = values[0].keys()                     
            set = pd.DataFrame(values, columns=columns)              
            excret(s3, bucket, path, set, keys_ingested, sep, m_schema.object_encoding(columns)) 
            del set
        elif util.time_remaining(context) <= 45:
            return
        del data  
        del df        
        del keys_ingested
            
        if util.elapsed(context) > timeout or mutil.get_process_memory_usage_bytes() >= c.ONE_GB_IN_BYTES:
            print "\tThe elapsed time threshold of {} seconds has been hit or the memory threshold of {} megabytes has been hit. Time: {}s, Memory: {}MB".format(timeout, c.ONE_GB_IN_BYTES/1048576.0, util.elapsed(context),mutil.get_process_memory_usage_megabytes())
            return           
    print "I've consumed everything I can in bucket '{}'".format(bucket)
    return

def excret(s3, bucket, key, data, keys_ingested, sep, object_encoding):
    output_filename = "{2}{0}{1}{0}combined.parquet".format(c.FILENAME_SEP, uuid.uuid1().hex, util.now_as_string() )
    output_path = "/{}/{}".format(key,output_filename)
    debug_print("\t\tSaving the ingested file to '{}'".format(output_path))
    
    writer.write(bucket, output_path, data, sep, object_encoding) 
    s3.delete(keys_ingested)
    

def calculate_aggregate_window_timeout(maximum_lambda_duration):     
    maximum_aggregation_period = maximum_lambda_duration * 0.9     
    return maximum_aggregation_period

def debug_print(message):        
    util.debug_print(message)