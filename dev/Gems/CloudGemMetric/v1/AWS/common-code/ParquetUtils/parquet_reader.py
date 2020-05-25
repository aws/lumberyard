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

from __future__ import print_function
from fastparquet import ParquetFile
from fastparquet import write as pwrite
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
        print(read(s3, bucket, key))

def debug_local_file(context, args):
    file_path = args.file_path
    print("Loading parquet file '{}'".format(file_path))
    pf = ParquetFile(file_path)
    df=pf.to_pandas()
    print("The set contains: \n{}".format(df))


