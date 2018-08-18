from cgf_utils import custom_resource_response
from sqs import Sqs
import json
import os
import metric_constant as c
import util

def handler(event, context):
    print "Start FIFO Auto Scale"    
    prefix = util.get_stack_name_from_arn(event[c.ENV_STACK_ID], False)        
    sqs = Sqs({}, queue_prefix=prefix)     
    request_type = event['RequestType']
    print request_type, prefix
    if request_type == 'Delete':        
        sqs.delete_all_queues(prefix)
    else:        
        queues = sqs.get_queues()      
        print queues   
        number_of_queues = len(queues)
        
        #5 queues to start, each queue can support 300 send message calls per second.  total: 1500 messages per second 
        for i in range(number_of_queues, 5):        
            sqs.add_fifo_queue(prefix)            

    return custom_resource_response.success_response({}, "*")

def main(context, args):      
    import util 
    resources = util.get_resources(context)   
    os.environ[c.ENV_REGION] = context.config.project_region
    
    handler({'RequestType': args.event_type, 'LogicalResourceId': "1234"}, type('obj', (object,), {'function_name' : resources[c.RES_LAMBDA_FIFOCONSUMER]}))
