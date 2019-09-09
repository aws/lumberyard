from cgf_utils import custom_resource_response
from sqs import Sqs
import json
import os
import metric_constant as c
import util

def handler(event, context):
    print "Start FIFO Auto Scale"    
    prefix = util.get_stack_name_from_arn(event[c.ENV_STACK_ID], False)            
    request_type = event['RequestType']    
    assigned_suffix = event['ResourceProperties'].get('Suffix', None)
    type = event['ResourceProperties'].get('QueueType', "fifo")
    initial_number_of_queues = int(event['ResourceProperties'].get('IntialNumberOfQueues', 5))
    
    if assigned_suffix:
        prefix = "{0}{1}".format(prefix,assigned_suffix)

    sqs = Sqs({}, queue_prefix=prefix, type=type)
    if request_type == 'Delete':
        sqs.delete_all_queues(prefix)
    else:
        queues = sqs.get_queues()
        number_of_queues = len(queues)
        
        #5 queues to start, each queue can support 300 send message calls per second.  total: 1500 messages per second 
        if number_of_queues < initial_number_of_queues:
            for i in range(number_of_queues, initial_number_of_queues):
                sqs.add_fifo_queue(prefix)

    return custom_resource_response.success_response({}, "*")

def main(context, args):      
    import util 
    resources = util.get_resources(context)   
    os.environ[c.ENV_REGION] = context.config.project_region
    
    handler({'RequestType': args.event_type, 'LogicalResourceId': "1234"}, type('obj', (object,), {'function_name' : resources[c.RES_LAMBDA_FIFOCONSUMER]}))
