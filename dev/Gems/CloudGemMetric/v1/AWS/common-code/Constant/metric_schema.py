import metric_constant as c
import collections

class Type(object):
    def __init__(self, id, type, long_name):
        self.__id = id  
        self.__type = type
        self.__long_name = long_name
    
    @property
    def id(self):
        return self.__id        

    @property
    def type(self):
        return self.__type

    @property
    def long_name(self):
        return self.__long_name

UUID = Type("uuid",'utf8','universal_unique_identifier')
EVENT = Type("event",'utf8', 'event_name')
SEQ_NO = Type("seqno",'int', 'event_sequence_number')
TIMEZONE_SIGN = Type("tzs",'utf8', 'timezone_sign')
TIMEZONE_HOUR = Type("tzh",'int', 'timezone_hour')
TIMEZONE_MIN = Type("tzm",'int', 'timezone_minute')
LOCALE = Type("loc",'utf8','locale')
UID = Type("uid",'infer','unique_user_identifier')
MESSAGE_ID = Type("msgid",'utf8', 'sqs_message_identifer')
BUILD_ID = Type("bldid",'utf8', 'client_build_identifier')  
PLATFORM_ID = Type("pltf",'utf8', 'platform_identifier')
CLIENT_TIMESTAMP = Type("tmutc",'utf8', 'client_timestamp')
SOURCE = Type("source",'utf8', 'event_source')
SERVER_TIMESTAMP = Type("srv_tmutc",'float','server_timestamp')
LONGITUDE = Type("longitude",'float', 'client_longitude')
LATITUDE = Type("latitude",'float', 'client_latitude')
COUNTRY_OF_ORIGIN = Type("cntry_origin",'utf8', 'client_country_of_origin')
SESSION_ID = Type("session_id",'utf8', 'client_session_id')
CUSTOM = Type("custom",'json','custom')
SENSITIVITY = Type("sensitivity",'json','data_sensitivity')
SCHEMA_HASH = Type("schema_hash",'json','event_schema_hash')

DICTIONARY = {
    UUID.id: UUID,
    EVENT.id: EVENT,
    SEQ_NO.id: SEQ_NO,
    TIMEZONE_SIGN.id: TIMEZONE_SIGN,
    TIMEZONE_HOUR.id: TIMEZONE_HOUR,
    TIMEZONE_MIN.id: TIMEZONE_MIN,
    LOCALE.id: LOCALE,
    UID.id: UID,
    MESSAGE_ID.id: MESSAGE_ID,
    BUILD_ID.id: BUILD_ID,
    PLATFORM_ID.id: PLATFORM_ID,
    CLIENT_TIMESTAMP.id: CLIENT_TIMESTAMP,
    SOURCE.id: SOURCE,
    SERVER_TIMESTAMP.id: SERVER_TIMESTAMP,
    LONGITUDE.id: LONGITUDE,
    LATITUDE.id: LATITUDE,
    COUNTRY_OF_ORIGIN.id: COUNTRY_OF_ORIGIN,
    SESSION_ID.id: SESSION_ID,
    CUSTOM.id: CUSTOM,
    SENSITIVITY.id: SENSITIVITY,
    SCHEMA_HASH.id: SCHEMA_HASH,
    UUID.long_name: UUID,
    EVENT.long_name: EVENT,
    SEQ_NO.long_name: SEQ_NO,
    TIMEZONE_SIGN.long_name: TIMEZONE_SIGN,
    TIMEZONE_HOUR.long_name: TIMEZONE_HOUR,
    TIMEZONE_MIN.long_name: TIMEZONE_MIN,
    LOCALE.long_name: LOCALE,
    UID.long_name: UID,
    MESSAGE_ID.long_name: MESSAGE_ID,
    BUILD_ID.long_name: BUILD_ID,
    PLATFORM_ID.long_name: PLATFORM_ID,
    CLIENT_TIMESTAMP.long_name: CLIENT_TIMESTAMP,
    SOURCE.long_name: SOURCE,
    SERVER_TIMESTAMP.long_name: SERVER_TIMESTAMP,
    LONGITUDE.long_name: LONGITUDE,
    LATITUDE.long_name: LATITUDE,
    COUNTRY_OF_ORIGIN.long_name: COUNTRY_OF_ORIGIN,
    SESSION_ID.long_name: SESSION_ID,    
    SENSITIVITY.long_name: SENSITIVITY,
    SCHEMA_HASH.long_name: SCHEMA_HASH
}

class Required(object):
    @staticmethod
    def event():
        return EVENT

    @staticmethod
    def seqno():
        return SEQ_NO

    @staticmethod
    def timezone_sign():
        return TIMEZONE_SIGN

    @staticmethod
    def timezone_hour():
        return TIMEZONE_HOUR

    @staticmethod
    def timezone_min():
        return TIMEZONE_MIN

    @staticmethod
    def locale():
        return LOCALE

    @staticmethod
    def uid():
        return UID

    @staticmethod
    def message_id():
        return MESSAGE_ID

    @staticmethod
    def build_id():
        return BUILD_ID

    @staticmethod
    def platform_id():
        return PLATFORM_ID

    @staticmethod
    def timestamp_utc():
        return CLIENT_TIMESTAMP

    @staticmethod
    def source():
        return SOURCE

    @staticmethod
    def session_id():
        return SESSION_ID

    @staticmethod
    def custom():
        return CUSTOM

    class Server(object):
        @staticmethod
        def server_timestamp_utc():
            return SERVER_TIMESTAMP

        @staticmethod
        def longtitude():
            return LONGITUDE

        @staticmethod
        def latitude():
            return LATITUDE

        @staticmethod
        def country():
            return COUNTRY_OF_ORIGIN

        @staticmethod
        def uuid():
            return UUID

    class RequestParameters(object):
        @staticmethod
        def sensitivity():
            return SENSITIVITY

        @staticmethod
        def schema_hash():
            return SCHEMA_HASH
    

#DO NOT CHANGE ORDER OR IT WILL BREAK YOUR GLUE TABLE SCHEMA
REQUIRED_ORDER = [ 
                Required.event(),
                Required.seqno(),
                Required.timezone_sign(),
                Required.timezone_hour(),
                Required.timezone_min(),
                Required.locale(),
                Required.uid(),
                Required.message_id(),
                Required.build_id(),
                Required.platform_id(),
                Required.timestamp_utc(),
                Required.source(),
                Required.session_id(),
                Required.Server.uuid(),
                Required.Server.longtitude(),
                Required.Server.latitude(),
                Required.Server.country(),
                Required.Server.server_timestamp_utc()
]

def object_encoding(columns):
    result = {}
    for col in columns:
        notfound = True
        for req in REQUIRED_ORDER:
            if col == req.id:
                notfound = False
                result[col] = req.type        
                break;
        if notfound:
            #custom columns must be defined as utf8 for now or we risk running into an Athena error HIVE_PARTITION_SCHEMA_MISMATCH: There is a mismatch between the table and partition schemas. The types are incompatible and cannot be coerced. 
            result[col] = 'infer'
        
    return result

def s3_key_format():
    return "table={5}{0}p_" + EVENT.long_name + "={5}{0}p_" + SERVER_TIMESTAMP.long_name + "_strftime={6}{0}p_" + SERVER_TIMESTAMP.long_name + "_year={1}{0}p_" + SERVER_TIMESTAMP.long_name + "_month={2}{0}p_" + SERVER_TIMESTAMP.long_name + "_day={3}{0}p_" + SERVER_TIMESTAMP.long_name + "_hour={4}"

class Order(object):
    def __init__(self):
        self.__order = REQUIRED_ORDER        

    @property
    def required_fields(self):
        return self.__order

    def order_columns(self, ordered_dict):
        dataset = ordered_dict.keys()
        result = []        
        for req in self.required_fields:
            result.append(req.id)
            
        #iterate the remaining fields
        custom = []
        for entry in dataset:
            if entry not in result:
                custom.append(entry)
        #sort non-required fields alpha
        custom.sort()        
        result.extend(custom)    

        return result


        
            
            
