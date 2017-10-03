import importlib
import boto3
import CloudCanvas
import errors
from datetime import datetime

message_table_name = CloudCanvas.get_setting('MessageTable')

message_table = boto3.resource('dynamodb').Table(message_table_name)

#Had to use these custom min and max rather than datetime.min because datetime.strttime will not accept a date prior to 1900
custom_datetime_min = 'Jan 1 1900 00:00'
custom_datetime_min_as_number = 190001010000

custom_datetime_max = 'Dec 31 2100 23:59'
custom_datetime_max_as_number = 210012312359

message_size_limit = 700

def get_message_table():
    if not hasattr(get_message_table, 'message_table'):
        message_table_name = CloudCanvas.get_setting('MessageTable')
        get_message_table.message_table = boto3.resource('dynamodb').Table(message_table_name)
        if get_message_table.message_table is None:
            raise RuntimeError('No Message Table')
    return get_message_table.message_table

#time utility functions
def _get_time_format():
    return '%b %d %Y %H:%M'
 
def get_struct_time(timestring):
    try:
        return datetime.strptime(timestring, _get_time_format())
    except ValueError:
        raise errors.ClientError('Expected time format {}'.format(get_formatted_time_string(datetime.utcnow())))
        
def get_formatted_time(timeval):
    return datetime.strftime(timeval, '%b %d %Y %H:%M')

def get_time_as_number(timestring):
    if timestring is None:
        timeStruct = datetime.utcnow()
        return int(datetime.strftime(timeStruct, '%Y%m%d%H%M'))

    timeStruct = get_struct_time(timestring)
    return int(datetime.strftime(timeStruct, '%Y%m%d%H%M'))

def get_struct_time_as_number(timestruct):
    return int(datetime.strftime(timestruct, '%Y%m%d%H%M'))


def get_formatted_time_from_number(timenum):
    year = int(timenum/100000000)
    remain = int(timenum - year*100000000) 
    month = int(remain/1000000)
    remain -= month*1000000
    day = int(remain/10000)
    remain -= day*10000
    hour = int(remain/100)
    minute = remain - hour*100
    d = datetime(year, month, day, hour, minute)
    return get_formatted_time(d);


def validate_start_end_times(start_timeval, end_timeval):
    #Scheduling with start and end time
    if get_struct_time(end_timeval) <= get_struct_time(start_timeval):
        raise errors.ClientError('Invalid: End time ' + end_timeval + ' <= Start time ' + start_timeval)

	#Scheduling with no end time is always valid	
    return True