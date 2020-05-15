import service
import message_utils
import errors
from botocore.exceptions import ClientError
from boto3.dynamodb.conditions import Attr
from datetime import datetime
from datetime import timedelta

@service.api
def get(request, time = None, lang = None):

    #Validate the time that was provided to avoid spoofing.
    if time is not None:
        client_datetime = message_utils.get_struct_time(time);
        current_time = datetime.utcnow()

        time_diff = current_time - client_datetime

        time_diff_in_hours = time_diff.total_seconds()/3600
        #No matter where you are in the world you should not have a time difference greater than 12 hours
        if time_diff_in_hours > 12 or time_diff_in_hours < -12 :
            print('Time diff is {}'.format(time_diff_in_hours))
            raise errors.ClientError('Invalid client time')

    #This function will return UTC if no timestring is provided
    client_time_as_number = message_utils.get_time_as_number(time)

    response = message_utils.get_message_table().scan(
            ProjectionExpression='message, priority, startTime, endTime',
		    FilterExpression=Attr('startTime').lte(client_time_as_number) & Attr('endTime').gte(client_time_as_number)
	    )

    data = []
    for i in response['Items']:
        conv = convert_table_entry(i)
        data.append(conv)
    return {
        "list": data
        }

def convert_table_entry(entry):
    message_object = {}
    message_object['message'] = entry.get('message', 'Undefined')
    message_object['startTime'] =  message_utils.get_formatted_time_from_number(entry.get('startTime', 'Undefined'))
    message_object['endTime'] =  message_utils.get_formatted_time_from_number(entry.get('endTime', 'Undefined'))

    if entry.get('priority') != None:
        message_object['priority'] =  int(entry['priority'])
    return message_object