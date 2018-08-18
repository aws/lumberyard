
from s3 import S3
from keyparts import KeyParts
import metric_constant as c
import time
import datetime
import sensitivity

VALID_ROOT_PATHS = [sensitivity.SENSITIVITY_TYPE.NONE,sensitivity.SENSITIVITY_TYPE.ENCRYPT]   

class Crawler(object):    

    def __init__(self, context, bucket):        
        self.__context = context        
        self.__s3 = S3(context, bucket)
        self.__bucket = bucket         
        self.__sep = context[c.KEY_SEPERATOR_PARTITION]

    @property
    def valid_root_paths(self):
        return VALID_ROOT_PATHS    

    def exists(self, prefix):                     
        for page in self.__s3.list(prefix=prefix):                        
            if "Contents" in page:
                for obj in page[ "Contents" ]:                    
                    return True
        return False

    def crawl(self, prefix, lambda_pool, func, depth=10):        
        #combine only files for the past X.  Older files should already be fully aggregated        
        print "Locating paths to crawl on bucket '{}' with prefix '{}'".format(self.__bucket, prefix)
        crawled_paths = {}
        idx = 0
        count = 0        
        for page in self.__s3.list(prefix=prefix):                        
            if "Contents" in page:
                for obj in page[ "Contents" ]:
                    key = obj['Key']                                                                        
                    parts = KeyParts(key, self.__sep)                                      
                    path = self.__sep.join(parts.path.split(self.__sep)[:depth])                     
                    if path not in crawled_paths:                       
                        crawled_paths[path]=True
                        func(self.__context, path, lambda_pool[idx])
                        count += 1
                        idx += 1                        
                        if idx >= len(lambda_pool):
                            idx = 0
        
        print "Path scouting complete on bucket '{}'".format(self.__bucket)
        return count  


    def crawl_from_relative(self, prefix):        
        #combine only files for the past two days.  Older files should already be fully aggregated
        start = datetime.datetime.utcnow() - datetime.timedelta(days=2)
        crawl_paths = dict({})
        for page in self.__s3.list(prefix):            
            if "Contents" in page:
                for obj in page[ "Contents" ]:
                    key = obj['Key']                                                    
                    parts = KeyParts(key, self.__sep)                    
                    event_date = datetime.datetime(parts.year, parts.month, parts.day, parts.hour)
                    if event_date >= start:
                        path = self.__sep.join(parts.path.split(self.__sep)[:-1])                          
                        if path not in crawl_paths: 
                            crawl_paths[path] = []
                        crawl_paths[path].append(parts.filename)                        
                        
        #assign an amoeba generator per identified path
        return crawl_paths     
