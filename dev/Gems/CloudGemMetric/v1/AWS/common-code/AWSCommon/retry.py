import random
import time
import metric_constant as c
from botocore.exceptions import ClientError

def try_with_backoff(context, cmd, **kwargs):
    # http://www.awsarchitectureblog.com/2015/03/backoff.html
    backoff = context[c.KEY_BACKOFF_BASE_SECONDS] if c.KEY_BACKOFF_BASE_SECONDS in context else 0.1    
    max_seconds = context[c.KEY_BACKOFF_MAX_SECONDS] if c.KEY_BACKOFF_MAX_SECONDS in context else 20.0
    max_retry = context[c.KEY_BACKOFF_MAX_TRYS] if c.KEY_BACKOFF_MAX_TRYS in context else 5
    count = 1
    while True:
        try:            
            return cmd(**kwargs)
        except ClientError as e:
            print e
            print "Command: {}".format(cmd)
            print "Args: {}".format(kwargs)
            if e.response['Error']['Code'] == 'ValidationException':  
                raise e
            backoff = min(max_seconds, random.uniform(max_seconds, backoff * 3.0))            
            time.sleep(backoff)
            count += 1
            if count > max_retry:
                raise e
                