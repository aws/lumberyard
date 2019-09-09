from fastparquet import ParquetFile
from fastparquet import write as pwrite
from StringIO import StringIO
import io
import metric_constant as c
import time
import traceback
import s3fs
import pandas as pd
import util
import time
import mem_util as mutil

def read(s3, bucket, key):      
    s3_open = s3.open    
    path1='{}{}'.format(bucket,key)
    pf1 = ParquetFile(path1, open_with=s3_open)    
    results = pf1.to_pandas()      
    return results

def debug_file(context, args):
    if args.file_path:
        debug_local_file(context,args)
    else:
        s3 = s3fs.S3FileSystem()        
        resources = util.get_resources(context)
        bucket = resources[c.RES_S3_STORAGE] 
        key = args.s3_key
        if not key.startswith('/'):
            key = "/{}".format(key)
        print read(s3,bucket,key)

def debug_local_file(context, args):
    file_path = args.file_path
    print "Loading parquet file '{}'".format(file_path)
    pf = ParquetFile(file_path)
    df=pf.to_pandas()
    print "The set contains: \n{}".format(df)   


