from StringIO import StringIO
from random import randint
from athena import DEFAULT_EVENTS
import os
import metric_constant as c
import metric_schema as schema
import string
import random
import time
import uuid
import json as json_util
import sys

class DataGenerator(object):
    """
        Generate test data for the metric producer
    """
    def __init__(self, context={}):
        self.__context = context 
        self.__source  = c.RES_GEM_NAME.lower()        
        self.__platforms = ['PC','iOS','OSX','Android']
        self.__buildid = ['1.0.2', '1.0.5432', '2.0.1']
        self.__loc = ['zh-cn','ar-dz', 'eu', 'en']        
        self.__session_id = [str(uuid.uuid1()), str(uuid.uuid1()), str(uuid.uuid1()), str(uuid.uuid1())]    
        self.__player_id = [str(uuid.uuid1()), str(uuid.uuid1()), str(uuid.uuid1()), str(uuid.uuid1())]    
        self.__events = [
                {"id":'dummy_defect', "attributes": [{"id":"log", "value": self.random_string}]},
                {"id":'dummy_shotFired', "attributes": [
                                        {"id":"gun", "value": self.random_string},
                                        {"id":"x", "value": self.random_float},
                                        {"id":"y", "value": self.random_float},
                                        {"id":"z", "value": self.random_float},
                                        {"id":"target", "value": self.random_string}                                        
                                    ]},
                {"id":'dummy_shotMissed', "attributes": [
                                        {"id":"gun", "value": self.random_string},
                                        {"id":"x", "value": self.random_float},
                                        {"id":"y", "value": self.random_float},
                                        {"id":"z", "value": self.random_float},
                                        {"id":"target", "value": self.random_string}
                                    ]},
                {"id":'dummy_dodged', "attributes":  [
                                        {"id":"attack_type", "value": self.random_string},
                                        {"id":"x", "value": self.random_float},
                                        {"id":"y", "value": self.random_float},
                                        {"id":"z", "value": self.random_float},
                                        {"id":"result", "value": self.random_string}
                                    ]},
                {"id":'dummy_bug', "attributes":  [
                                        {"id":"error_code", "value": self.random_int},
                                        {"id":"message", "value": self.random_string},
                                        {"id":"x", "value": self.random_float},
                                        {"id":"y", "value": self.random_float},
                                        {"id":"z", "value": self.random_float},
                                        {"id":"stack", "value": self.random_string}
                                    ]},
                {"id": "dummy_"+DEFAULT_EVENTS.CLIENTINITCOMPLETE, "attributes":  []},
                {"id": "dummy_"+DEFAULT_EVENTS.SESSIONSTART, "attributes":  []},
                {"id": 'dummy_json', "attributes":  [
                                        {"id":"complex_obj", "value": {
                                                                "json_1":1,
                                                                "json_2":2
                                                            }}
                                    ]},
                {"id":'dummy_array', "attributes":  [
                                        {"id":"array_obj", "value": [
                                                                "item1",
                                                                "item2"
                                                            ]}
                                    ]}
            ]
                

    @property
    def uuid_hex(self):
        return uuid.uuid1().hex 

    @property
    def uuid(self):
        return uuid.uuid1()
    
    @property
    def random_string(self, size=20):          
        return ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(0, size))

    @property
    def random_json(self):          
        return { self.random_string: self.random_string }

    @property
    def random_float(self):          
        return random.uniform(0.0, 300.0)

    @property
    def random_int(self):          
        return random.randint(0,sys.maxint)

    @property
    def defect(self):
        return self.__events[0]    

    @property
    def shotfired(self):
        return self.__events[1]    

    @property
    def shotmissed(self):
        return self.__events[2]    

    @property
    def shotdodged(self):
        return self.__events[3]    
    
    @property
    def bug(self):
        return self.__events[4]    

    @property
    def clientinit(self):
        return self.__events[5] 

    @property
    def sessionstart(self):
        return self.__events[6] 

    #NOT CURRENTLY SUPPORTED
    #@property
    #def event_json(self):
    #    return self.__events[7] 
    
    #@property
    #def event_array(self):
    #    return self.__events[8] 

    @property
    def random_event(self):
        #do not send event_json and event_array as they are not supported by GLUE and or PARQUET
        return self.__events[randint(0, len(self.__events)-3)]

    def csv(self, number_of_events, event_type='random_event'):       
        print "Generating CSV test data with {} events.".format(number_of_events)        
        event = eval('self.{}'.format(event_type))
        
        csv = StringIO()
        header = [                
                schema.Required.event().id,
                schema.Required.seqno().id,
                schema.Required.timezone_sign().id,
                schema.Required.timezone_hour().id,
                schema.Required.timezone_min().id,
                schema.Required.locale().id,
                schema.Required.uid().id,
                schema.Required.message_id().id,
                schema.Required.build_id().id,
                schema.Required.platform_id().id,
                schema.Required.timestamp_utc().id,
                schema.Required.source().id,
                schema.Required.session_id().id                
            ]   
        custom = []
        for attr in event["attributes"]:
            header.append(attr["id"])          
            custom.append(str(attr["value"]))
        header = self.__context[c.KEY_SEPERATOR_CSV].join(header)              
        custom = self.__context[c.KEY_SEPERATOR_CSV].join(custom)        
        csv.write(header+c.NEW_LINE)   
               
        for i in range(0,number_of_events):                        
            csv.write("{}{}".format(event["id"],self.__context[c.KEY_SEPERATOR_CSV])) 
            csv.write("{}{}".format(i,self.__context[c.KEY_SEPERATOR_CSV]))
            csv.write("-{}".format(self.__context[c.KEY_SEPERATOR_CSV]))
            csv.write("3{}".format(self.__context[c.KEY_SEPERATOR_CSV]))
            csv.write("0{}".format(self.__context[c.KEY_SEPERATOR_CSV]))
            csv.write("{}{}".format(self.__loc[randint(0, len(self.__loc)-1)],self.__context[c.KEY_SEPERATOR_CSV]))
            csv.write("{}{}".format(self.__player_id[randint(0, len(self.__player_id)-1)],self.__context[c.KEY_SEPERATOR_CSV]))
            csv.write("{}{}".format(str(self.uuid),self.__context[c.KEY_SEPERATOR_CSV]))
            csv.write("{}{}".format(self.__buildid[randint(0, len(self.__buildid)-1)],self.__context[c.KEY_SEPERATOR_CSV]))
            csv.write("{}{}".format(self.__platforms[randint(0, len(self.__platforms)-1)],self.__context[c.KEY_SEPERATOR_CSV]))
            csv.write("{}{}".format(time.time(),self.__context[c.KEY_SEPERATOR_CSV]))        
            csv.write("{}{}".format(self.__source,self.__context[c.KEY_SEPERATOR_CSV]))
            if len(custom) == 0:
                csv.write("{}".format(self.__session_id[randint(0, len(self.__session_id)-1)]))
            else:
                csv.write("{}{}".format(self.__session_id[randint(0, len(self.__session_id)-1)],self.__context[c.KEY_SEPERATOR_CSV]))
            csv.write(custom+c.NEW_LINE)            
              
        result = csv.getvalue()            
        csv.close()          
        return result

    def json(self, number_of_events, event_type='random_event'):       
        print "Generating JSON test data with {} events.".format(number_of_events)
        set = []
        event = eval('self.{}'.format(event_type))
        
        json = {}
        for attr in event["attributes"]:
            json[attr["id"]]=attr["value"]
        for i in range(0,number_of_events):                        
            json[schema.Required.event().id] = event["id"]
            json[schema.Required.seqno().id] = i
            json[schema.Required.timezone_sign().id] = "-"
            json[schema.Required.timezone_hour().id] = 3
            json[schema.Required.timezone_min().id] = 0
            json[schema.Required.locale().id] = self.__loc[randint(0, len(self.__loc)-1)]
            json[schema.Required.uid().id] = self.__player_id[randint(0, len(self.__player_id)-1)]
            json[schema.Required.message_id().id] = str(self.uuid)
            json[schema.Required.build_id().id] = self.__buildid[randint(0, len(self.__buildid)-1)]
            json[schema.Required.platform_id().id] = self.__platforms[randint(0, len(self.__platforms)-1)]
            json[schema.Required.timestamp_utc().id] = time.time()
            json[schema.Required.source().id] = self.__source
            json[schema.Required.session_id().id] = self.__session_id[randint(0, len(self.__session_id)-1)]              
        
            set.append(json)
        return json_util.dumps(set)