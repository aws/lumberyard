from fastparquet import ParquetFile
from fastparquet import write as pwrite
from StringIO import StringIO
from keyparts import KeyParts
import sensitivity
import io
import metric_constant as c
import time
import traceback
import s3fs
import pandas as pd
import numpy as np
import util
import time
import metric_schema as schema
import metric_error_code as error
import parquet_reader
import uuid
import os
import gzip
import json
import boto3
import math
import sys
import datetime
import gc
from s3 import S3
from aws_lambda import Lambda
from keyparts import KeyParts
from datetime import date, timedelta

s3fsmap = {
    sensitivity.SENSITIVITY_TYPE.ENCRYPT.lower(): s3fs.S3FileSystem(config_kwargs={'signature_version': 's3v4'}, s3_additional_kwargs={'ServerSideEncryption': 'AES256'}),
    sensitivity.SENSITIVITY_TYPE.NONE.lower(): s3fs.S3FileSystem(),
}

def write(bucket, key, data, sep, object_encoding, append=False):   
    if data.empty:        
        raise RuntimeError( "[{}]An attempt to write an empty dataset has occurred.  The request dataset was: {}".format(error.Error.empty_dataframe(), data))    
    sensitivity_type = KeyParts(key, sep).sensitivity_level.lower()   
    s3 = s3fsmap[sensitivity_type]    
    s3_open = s3.open    
    size_before_dup_drop = len(data)
    data.drop_duplicates(inplace=True)        
    size_after_dup_drop = len(data)        
    if size_before_dup_drop - size_after_dup_drop > 0:
        print "{} duplicates have been dropped".format(size_before_dup_drop - size_after_dup_drop) 
    util.debug_print("Using object encoding {}".format(object_encoding))
    path='{}{}'.format(bucket,key)          
    pwrite(path, data, open_with=s3_open, compression='GZIP', append=append, has_nulls=True, object_encoding=object_encoding)        
    return path
    
def append(bucket, key1, key2, s3, output_filename):  
    s3_open = s3.open
    path1='{}{}'.format(bucket,key1)   
    pf1 = ParquetFile(path1, open_with=s3_open)
    df1=pf1.to_pandas()
    path2='{}{}'.format(bucket,key2)   
    pf2 = ParquetFile(path2, open_with=s3_open)
    df2=pf2.to_pandas()            
    data = df1.append(df2) 
    
    pwrite('{}{}'.format(bucket,output_filename), data, open_with=s3_open, compression='GZIP', append=False, has_nulls=True)    

def save(context, metric_sets, partition, schema_hash):
    util.debug_print("\t{}:".format(partition))
    paths = []
    for schema_hash, dict  in metric_sets[partition].iteritems():  
        if util.time_remaining(context) <= (context[c.CW_ATTR_DELETE_DURATION] + 20):
            break
        columns = dict[c.KEY_SET_COLUMNS]        
        if len(dict[c.KEY_SET]) == 0:
            continue
        values = dict[c.KEY_SET].values()
        set = pd.DataFrame(values, columns=columns)    
        
        util.debug_print("\t\t{}:".format(schema_hash))
        path = create_file_path(partition, schema_hash, context[c.KEY_SEPERATOR_PARTITION])              
        paths.append(path)
        util.debug_print("Writing to path '{}' with set:\n {}".format(path, set))                
        elapsed = 0        
        try:            
            util.debug_print("Writing to partition '{}'".format(partition))                           
            write(context[c.KEY_METRIC_BUCKET], path, set, context[c.KEY_SEPERATOR_PARTITION],schema.object_encoding(columns))            
            handoff_event_to_emitter(context, context[c.KEY_METRIC_BUCKET], path, set)
            context[c.KEY_SUCCEEDED_MSG_IDS] += dict[c.KEY_MSG_IDS]
            util.debug_print("Save complete to path '{}'".format(path))            
        except Exception as e:                        
            print "[{}]An error occured writing to path '{}'.\nSet: {}\nError: \n{}".format(context[c.KEY_REQUEST_ID],path, set, traceback.format_exc())            
            raise e
        finally:                        
            number_of_rows = len(values)           
            if c.INFO_TOTAL_ROWS not in context[c.KEY_AGGREGATOR].info:
                context[c.KEY_AGGREGATOR].info[c.INFO_TOTAL_ROWS] = 0
            context[c.KEY_AGGREGATOR].info[c.INFO_TOTAL_ROWS] += number_of_rows            
            del set
            del values
            del columns
            gc.collect()
    try:   
        util.debug_print("Adding amoeba message to SQS queue '{}'".format(context[c.KEY_SQS_AMOEBA].queue_url))        
        context[c.KEY_SQS_AMOEBA].send_generic_message(json.dumps({ "paths": paths }))
    except Exception as e:                        
        print "[{}]An error occured writing messages to the Amoeba SQS queue. \nError: \n{}".format(context[c.KEY_REQUEST_ID], traceback.format_exc())            
        raise e

def handoff_event_to_emitter(context, bucket, key, events):   
    bucket = os.environ["ProjectConfigurationBucket"]
    lmdclient = Lambda(context)
    s3client = S3(context, bucket)    
    
    parts = KeyParts(key, context[c.KEY_SEPERATOR_PARTITION]) 
    key = "deployment/share/emitted_event_payloads/{}/{}/{}/{}".format( parts.source, parts.event, parts.datetime, parts.filename.replace(parts.extension, 'json'))
    
    payload = {
                    'emitted': {
                        'key': key,
                        'bucket': bucket,
                        'type': parts.event,
                        'source': parts.source,
                        'buildid': parts.buildid,
                        'filename': parts.filename.replace(parts.extension, 'json'),
                        'datetime': parts.datetime,
                        'datetimeformat': util.partition_date_format(),
                        'sensitivitylevel': parts.sensitivity_level
                    }
            }    
    
    #create a temporary file for the event emitter to read
    expires = datetime.datetime.utcnow() + datetime.timedelta(minutes=30)
    s3client.put_object(
        key,
        events.to_json(orient='records'),
        expires           
    )    

    resp = lmdclient.invoke(
        os.environ[c.ENV_EVENT_EMITTER],
        payload        
    )

def write_geo_files(context, args):
    resources = util.get_resources(context)    
    s3_bucket = resources[c.RES_S3_STORAGE]
    s3 = s3fs.S3FileSystem()  
    s3_open = s3.open
    geo_data_files = [
            "GeoLite2/GeoLite2_Blocks/IPv4/GeoLite2-Country-Blocks-IPv4.csv",
            "GeoLite2/GeoLite2_Blocks/IPv6/GeoLite2-Country-Blocks-IPv6.csv",
            "GeoLite2/GeoLite2_Locations/GeoLite2-Country-Locations-de.csv",
            "GeoLite2/GeoLite2_Locations/GeoLite2-Country-Locations-en.csv",
            "GeoLite2/GeoLite2_Locations/GeoLite2-Country-Locations-es.csv",
            "GeoLite2/GeoLite2_Locations/GeoLite2-Country-Locations-fr.csv",
            "GeoLite2/GeoLite2_Locations/GeoLite2-Country-Locations-ja.csv",
            "GeoLite2/GeoLite2_Locations/GeoLite2-Country-Locations-pt-BR.csv",
            "GeoLite2/GeoLite2_Locations/GeoLite2-Country-Locations-ru.csv",
            "GeoLite2/GeoLite2_Locations/GeoLite2-Country-Locations-zh-CN.csv",
            "GeoLite2/GeoLite2-COPYRIGHT.txt",
            "GeoLite2/GeoLite2-LICENSE.txt",
            "OpenStreetMap/level_2_polygons.json.gz"
            ]
    
    cwd = os.getcwd()
    bucket = "s3://{}".format(s3_bucket)

    for path in geo_data_files:
        parts = path.split(".")
        path_with_filename = parts[0]

        extension = parts[1]
        if len(parts) == 3:
            extension = parts[2]

        rel_path = "Gems/CloudGemMetric/v1/{}".format(path)                
        if extension == 'gz':
            file_object = gzip.open(rel_path)
            extension = parts[1]                                                
        else:
            file_object = open(rel_path, "r")
        content = file_object.read() 
        write_file(content, bucket, path, s3, path_with_filename, extension)
        
def write_file(content, bucket, path, s3, path_with_filename, extension):
    if extension == "csv":
        df = pd.read_csv(StringIO(content),sep=",", encoding="utf-8") 
        target = "{}/{}.{}".format(bucket,path,"parquet")
        print path, "--->", target
        pwrite(target, df, open_with=s3.open, compression='GZIP', append=False, has_nulls=True)  
    elif extension == "json":
        obj = json.loads(content)    
        parts = path.split("/")     
        filename = parts[len(parts)-1].split(".")[0]       
        for feature in obj['features']:
            #if feature['name'] == "United States of America" or feature['name'] == "Canada"  :             
            if 'ISO3166-1' in feature['properties']:                
                geometry={}
                iso_code = feature['properties']['ISO3166-1']
                geometry["type"] = feature["geometry"]["type"]
                geometry["coordinates"] = feature["geometry"]["coordinates"]
                data = json.dumps(geometry, separators=(',', ':'))
                target = "{}/{}/p_iso3166={}/{}.json".format(bucket, parts[0], iso_code, filename)
                print path, "--->", target
                with s3.open(target, 'wb') as f:
                    f.write(data)                
       
    elif extension == "gz":
        with s3.open("{}/{}.json".format(bucket, path_with_filename), 'wb') as f:
            f.write(content)
    else:
        target = "{}/{}".format(bucket, path)
        print path, "--->", target
        with s3.open(target, 'wb') as f:
            f.write(content)

def create_file_path(partition, schema, sep):
    path_without_filename = util.path_without_filename(partition,schema,sep)      
    return util.path_with_filename(partition, util.now_as_string(), uuid.uuid1().hex, sep)    

if __name__ == '__main__':
    debug_file()
            