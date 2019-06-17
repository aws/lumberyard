import random
import time
import metric_constant as c
import util
from botocore.exceptions import ClientError

def try_with_backoff(context, cmd, **kwargs):
    # http://www.awsarchitectureblog.com/2015/03/backoff.html
    backoff = context[c.KEY_BACKOFF_BASE_SECONDS] if c.KEY_BACKOFF_BASE_SECONDS in context else 0.1    
    max_seconds = context[c.KEY_BACKOFF_MAX_SECONDS] if c.KEY_BACKOFF_MAX_SECONDS in context else 20.0
    max_retry = context[c.KEY_BACKOFF_MAX_TRYS] if c.KEY_BACKOFF_MAX_TRYS in context else 5
    count = 1    
    while True:
        try:            
            response = cmd(**kwargs)
            __check_response(response)
            util.debug_print("{}\n{}".format(cmd, kwargs))
            util.debug_print(response)
            return response
        except ClientError as e:
            __print_error(e, cmd, kwargs)
            if e.response['Error']['Code'] == 'ValidationException':  
                raise e           
        except Exception as general_error:
            __print_error(general_error, cmd, kwargs)
        
        backoff = min(max_seconds, random.uniform(max_seconds, backoff * 3.0))            
        util.debug_print("Backing off for {}s".format(backoff))
        time.sleep(backoff)
        count += 1
        if count > max_retry:
            print response
            raise Exception("Max retry attempts have been made")
                
def __print_error(error, cmd, kwargs):
    print error
    print "Command: {}".format(cmd)
    print "Args: {}".format(kwargs) 

def __check_response(response):
    #we should apply a chain of command pattern here if we find more custom retry cases
    #SQS batch failure            
    sqs_error = response.get("Failed", None)
    if sqs_error is not None:
        raise Exception(response)

    #s3 batch failure
    s3_error = response.get("Errors", [])
    if len(s3_error) > 0:
        raise Exception(response)