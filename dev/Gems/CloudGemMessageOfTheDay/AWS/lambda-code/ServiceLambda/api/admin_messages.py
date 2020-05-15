import service
import message_utils
import errors
import uuid
from datetime import datetime
from datetime import timedelta
from botocore.exceptions import ClientError
from boto3.dynamodb.conditions import Attr

#****************************************************************
#POST to add a new message
#****************************************************************
@service.api
def post(request, msg):

    rand_uuid = uuid.uuid4()
    unique_msg_id = rand_uuid.hex
    message = msg.get('message')

    if message is None:
        raise errors.ClientError('Value message cannot be None')

    if len(message) > message_utils.message_size_limit:
        raise errors.ClientError('Maximum message size is {} UTF8 encoded characters'.format(message_utils.message_size_limit))

    priority = msg.get('priority', 0)
    
    #If no start time specified set it to the min to deactivate filtering based on start
    start_time = msg.get('startTime', message_utils.custom_datetime_min)
    
    #If no start time specified set it to the max to deactivate filtering based on end
    end_time = msg.get('endTime', message_utils.custom_datetime_max)

    message_utils.validate_start_end_times(start_time, end_time)
    
    start_time_as_number = message_utils.get_time_as_number(start_time)
    end_time_as_number = message_utils.get_time_as_number(end_time)
    message_utils.get_message_table().put_item(
        Item={
        'UniqueMsgID': unique_msg_id,
        'message': message,
        'priority' : priority,
        'startTime' : start_time_as_number,
        'endTime' : end_time_as_number
		}
	)
    # As opposed to the item that was added to the DB the return object uses the human readable time format
    returnObj={
        'UniqueMsgID': unique_msg_id,
        'message': message,
        'priority' : priority,
        'startTime' : start_time,
        'endTime' : end_time
    }
    return returnObj
 
#****************************************************************
#PUT to edit an existing message
#****************************************************************
@service.api
def put(request, msg_id, msg):

    if msg_id is None:
        raise errors.ClientError('Value msg_id cannot be None')

    unique_msg_id = msg_id
    message = msg.get('message')

    if len(message) > message_utils.message_size_limit:
        raise errors.ClientError('Maximum message size is {} UTF8 encoded characters'.format(message_utils.message_size_limit))

    priority = msg.get('priority', 0)
    #If no start time specified set it to the min to deactivate filtering based on start
    start_time = msg.get('startTime', message_utils.custom_datetime_min)
    
    #If no end time specified set it to the max to deactivate filtering based on end
    end_time = msg.get('endTime', message_utils.custom_datetime_max)

    message_utils.validate_start_end_times(start_time, end_time)
    
    try:
        table = message_utils.get_message_table()
        start_time_as_number = message_utils.get_time_as_number(start_time)
        end_time_as_number = message_utils.get_time_as_number(end_time)
        
        if message is not None:
            table.update_item(
                Key={
                    'UniqueMsgID': unique_msg_id
                },
                UpdateExpression='SET message = :val1, priority = :val2, startTime = :val3, endTime = :val4',
                ConditionExpression='UniqueMsgID = :val',
                ExpressionAttributeValues={
                    ':val' : unique_msg_id,
                    ':val1': message,
                    ':val2': priority,
                    ':val3': start_time_as_number,
                    ':val4': end_time_as_number
                }
            )
        #Any value has a default so if you are doing a PUT if the Message is none then we want to leave it as is
        #in other words for all the other fields in the table if you send a None they will get reset to the default 
        else:
            table.update_item(
                Key={
                    'UniqueMsgID': unique_msg_id
                },
                UpdateExpression='SET priority = :val1, startTime = :val2, endTime = :val3',
                ConditionExpression='UniqueMsgID = :val',
                ExpressionAttributeValues={
                    ':val' : unique_msg_id,
                    ':val1': priority,
                    ':val2': start_time_as_number,
                    ':val3': end_time_as_number
                }
            )
    except ClientError as e:
        #Our unique message ID was not found
        if e.response['Error']['Code'] == 'ConditionalCheckFailedException':
            raise errors.ClientError('Invalid message id: {}'.format(unique_msg_id))
        raise e
    
    return 'MessageUpdated'

#****************************************************************
#DELETE to delete an existing message
#****************************************************************
@service.api
def delete(request, msg_id):
    try:
        response = message_utils.get_message_table().delete_item(
            Key={
                'UniqueMsgID': msg_id
            },
            ConditionExpression='UniqueMsgID = :val',
            ExpressionAttributeValues= {
                ":val": msg_id
            }
	    )
    except ClientError as e:
        #Our unique message ID was not found
        if e.response['Error']['Code'] == 'ConditionalCheckFailedException':
            raise errors.ClientError('Invalid message id: {}'.format(msg_id))
        raise e

    return 'MessageDeleted'

#****************************************************************
#GET to retrieve the detailed list of messages
#****************************************************************
@service.api
def get(request, index = None, count = None, filter = None):
    start_index = 0
    if index is not None:
        start_index = index
    page_size = 9999999
    if count is not None:
        page_size = count

    end_index = start_index + page_size

    print('Start index : {} Page size : {} end index : {}'.format(start_index, page_size, end_index))

    table = message_utils.get_message_table()

    
    current_time = datetime.utcnow()
    active_time_lower = message_utils.get_struct_time_as_number(current_time - timedelta(hours=12))
    active_time_upper = message_utils.get_struct_time_as_number(current_time + timedelta(hours=12))

    do_filter = False
    lc_filter = filter.lower()
    fe = ''
    if lc_filter == 'active':
        fe = Attr('endTime').gte(active_time_lower) & Attr('startTime').lte(active_time_upper)
        do_filter = True
    elif lc_filter == 'expired':
        fe = Attr('endTime').lt(active_time_lower)
        do_filter = True
    elif lc_filter == 'planned':
        fe = Attr('startTime').gt(active_time_upper)
        do_filter = True
    else:
        lc_filter = 'invalid'

    if do_filter == True:
        response = table.scan(
            FilterExpression=fe
            )
    else:
        response = table.scan()

    data = []
    current_index = 0
    respLength = len(response['Items'])
    print("Response length : {}".format(respLength))
    #First test if there is something for us in this scan
    if start_index < respLength:
        for i in response['Items']:
            if current_index >= start_index:
                print("Appending index: {}".format(current_index))
                conv = convert_table_entry(i)
                data.append(conv)
            current_index += 1
            if current_index >= end_index:
                return  {
                    "list": data
                    }
    #if not skip this whole loop and increment the current_index consequently
    else:
        current_index += len(response['Items'])

    while 'LastEvaluatedKey' in response:
        print("Looping")
        if do_filter == True:
            response = table.scan(
                ExclusiveStartKey=response['LastEvaluatedKey'],
                FilterExpression=fe
                )
        else:
            response = table.scan(ExclusiveStartKey=response['LastEvaluatedKey'])

        #First test if there is something for us in this scan
        if start_index < current_index + len(response['Items']):
            for i in response['Items']:
                if current_index >= start_index:
                    print("Appending index: {}".format(current_index))
                    conv = convert_table_entry(i)
                    data.append(conv)
                current_index += 1
                if current_index >= end_index:
                    return  {
                        "list": data
                        }
        #if not skip this whole loop and increment the current_index consequently
        else:
            current_index += len(response)

    return  {
        "list": data
        }

def convert_table_entry(entry):
    message_object = {}
    message_object['UniqueMsgID'] =  entry.get('UniqueMsgID', 'Undefined')
    message_object['message'] = entry.get('message', 'Undefined')

    if entry.get('priority') != None:
        message_object['priority'] =  int(entry['priority'])

    start_time = entry.get('startTime', message_utils.custom_datetime_min_as_number)
    if start_time != message_utils.custom_datetime_min_as_number:
        message_object['startTime'] =  message_utils.get_formatted_time_from_number(start_time)

    end_time = entry.get('endTime', message_utils.custom_datetime_max_as_number)
    if end_time != message_utils.custom_datetime_max_as_number:
        message_object['endTime'] =  message_utils.get_formatted_time_from_number(end_time)

    return message_object
