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

import service
import os
import metric_constant as c
import errors
from dynamodb import DynamoDb
from decimal import Decimal

@service.api
def get(request, filter=[]):    
    db = DynamoDb()
    result = db.get(db.context_table)
    response = dict({})
    if result and len(result['Items']) > 0:
        for pair in result['Items']:
            if pair[c.KEY_PRIMARY] in filter or len(filter)==0: 
                response[pair[c.KEY_PRIMARY]]= pair[c.KEY_SECONDARY]            
    
    return response

@service.api
def get_settings(request, data = None):        
    result = get(request)
    for entry in c.NON_SETTINGS:                
        del result[entry]    
    return result

@service.api
def update_settings(request, data = None):       
    for entry in data:        
        if entry in c.NON_SETTINGS:
            raise errors.ClientError("The '{}' key can not be updated through this API.  It is an object type and must be updated using its corresponding service API.".format(entry))     
        
    update_context(request, data)
    return data

@service.api
def get_predefined_partition(request):
    partitions = get(request, [c.KEY_PARTITIONS])[c.KEY_PARTITIONS]
    predefined_partitions = []
    for partition in partitions:
        key = partition['key']
        if key in c.PREDEFINED_PARTITIONS:
            predefined_partitions.append(partition)        
    return predefined_partitions

@service.api
def get_custom_partition(request):
    partitions = get(request, [c.KEY_PARTITIONS])[c.KEY_PARTITIONS]   
    custom_partitions = [] 
    for partition in partitions:
        key = partition['key']
        if key not in c.PREDEFINED_PARTITIONS:
            custom_partitions.append(partition)        
    return custom_partitions

@service.api
def get_partition(request):
    return get(request, [c.KEY_PARTITIONS])

@service.api
def get_client_context(request):
    return get(request, [c.CLIENT_BUFFER_FLUSH_TO_FILE_IN_BYTES, 
                        c.CLIENT_BUFFER_GAME_FLUSH_PERIOD_IN_SEC, 
                        c.CLIENT_FILE_NUM_METRICS_TO_SEND_IN_BATCH, 
                        c.CLIENT_FILE_SEND_METRICS_INTERVAL_IN_SECONDS, 
                        c.CLIENT_FILE_MAX_METRICS_TO_SEND_IN_BATCH_IN_MB, 
                        c.CLIENT_FILE_PRIORITIZATION_THRESHOLD_IN_PERC,
                        c.KEY_PRIORITIES,
                        c.KEY_FILTERS
                        ])

@service.api
def update_custom_partition(request, data = None):        
    if len(data) != 1:
        raise errors.ClientError("An attempt to update more than just the partitions has been made.  The length of the data set exceeds 1.")

    if c.KEY_PARTITIONS not in data:
        raise errors.ClientError("The '{}' key is not present in the data '{}'.".format(c.KEY_PARTITIONS, data))

    for partition in data['partitions']:
        if 'key' not in partition:
            raise errors.ClientError("The partition '{}' is missing the attribute 'key'.".format(partition))      
        if 'type' not in partition:
            raise errors.ClientError("The partition '{}' is missing the attribute 'type'.".format(partition))      
        
        key = partition['key']
        if key in c.PREDEFINED_PARTITIONS:
            raise errors.ClientError("The '{}' partition is a predefined partition and can't be updated with this API.".format(key))              
    predefined = get_predefined_partition(request)
    data[c.KEY_PARTITIONS] = predefined + data[c.KEY_PARTITIONS]
    update_context(request, data)
    return data

@service.api
def get_filter(request):
    return get(request, [c.KEY_FILTERS])

@service.api
def update_filter(request, data = None):    
    if len(data) != 1:
        raise errors.ClientError("An attempt to update more than just the filter has been made.  The length of the data set exceeds 1.")

    if c.KEY_FILTERS not in data:
        raise errors.ClientError("The '{}' key is not present in the data '{}'.".format(c.KEY_FILTERS, data))

    for filter in data[c.KEY_FILTERS]:
        if 'event' not in filter:
            raise errors.ClientError("The filter '{}' is missing the attribute 'event'.".format(filter))      
        if 'attributes' not in filter:
            raise errors.ClientError("The filter '{}' is missing the attribute 'attributes'.".format(filter))      
        if 'type' not in filter:
            raise errors.ClientError("The filter '{}' is missing the attribute 'type'.".format(filter))                              
        
    update_context(request, data)
    return data

@service.api
def get_priority(request):
    return get(request, [c.KEY_PRIORITIES])

@service.api
def update_priority(request, data = None):    
    if len(data) != 1:
        raise errors.ClientError("An attempt to update more than just the priority has been made.  The length of the data set exceeds 1.")

    if c.KEY_PRIORITIES not in data:
        raise errors.ClientError("The '{}' key is not present in the data '{}'.".format(c.KEY_PRIORITIES, data))

    for priority in data[c.KEY_PRIORITIES]:
        if 'event' not in priority:
            raise errors.ClientError("The priority '{}' is missing the attribute 'event'.".format(priority))      
        if 'precedence' not in priority:
            raise errors.ClientError("The priority '{}' is missing the attribute 'attributes'.".format(priority))             
    update_context(request, data)
    return data

def update_context(request, data = None):
    db = DynamoDb()
    for item in data:
        key = item
        value = data[item]
        print "Updating '{}' with value '{}'".format(key, value)
        params = dict({})
        params["UpdateExpression"]= "SET #val = :val"       
        params["ExpressionAttributeNames"]={
            '#val': c.KEY_SECONDARY                      
        }
        params["ExpressionAttributeValues"]={                    
            ':val': value
        }                 
        params["Key"]={ 
            c.KEY_PRIMARY: key
        }          
        try:
            db.update(db.context_table.update_item, params)        
        except Exception as e:
            raise errors.ClientError("Error updating the context parameter '{}' with value '{}'.\nError: {}".format(key, value, e))
        
    return data

def cli(context, args):
    #this import is only available when you execute via cli
    import util
    resources = util.get_resources(context)        
    os.environ[c.ENV_DB_TABLE_CONTEXT] = resources[c.RES_DB_TABLE_CONTEXT]   
    os.environ[c.ENV_REGION] = context.config.project_region
    
    print eval(args.function)( type('obj', (object,), {c.ENV_STACK_ID: resources[c.ENV_STACK_ID]}))

