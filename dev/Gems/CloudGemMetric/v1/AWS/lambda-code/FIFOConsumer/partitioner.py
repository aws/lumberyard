import datetime
import metric_constant as c
import metric_schema
from metric_schema import Type

class Partitioner(object):
    """
        A partition is a segment of a dataset.  The Cloud Gem Portal defines what the partitions should be.
        Based on the partition definition a S3 key is returned.  Order of the partition definitions matters.

        Example:
        
        Input:
        Partition Definition:
        [
            {key:"tmutc",type:"datetime",parts:['.year','.month','.day','.hour']},
            {key:"event",type:"str",parts:[]},
        ]        
        
        Data:  
                bldid       event gun loc                             msgid   pltf seqno  \
               1.0.11   shotFired  D2  en  d7649fa5a9ff11e79a3b6480998557f4  win64     1
              
                      tmutc tzh tzm tzs                               uid  
               1.507230e+09   3   0   -  d7649fa1a9ff11e7914c6480998557f4
            
        Output: 
            2017/10/03/14/shotFired

    """

    def __init__(self, definitions, seperator):                  
        self.__definitions = definitions
        self.__sep = seperator
        self.__map = {}
        self.__sanitization_list = (('\'',''),('"',''),('\\',''))
    
    @property
    def partitions(self):
        return self.__definitions;

    def extract(self, schema_hash, row, sensitivity):
        partition = ''
        tablename = None 
        self.__map['schema_hash'] = schema_hash
        self.__map['sensitivity'] = sensitivity.identifier         
        for definition in self.__definitions:                     
            key = definition["key"].lower()       
            key = metric_schema.DICTIONARY[key] if key in metric_schema.DICTIONARY else Type(key,'infer', key)
            type = definition["type"]        
            parts = definition["parts"] if "parts" in definition else [] 
            if key.id not in row and key.long_name not in row and key.id not in self.__map and key.long_name not in self.__map:
                print "ROW DROPPED: The partition definition with key '{}' (long name: '{}') was not found in the data row \n{}".format(key.id, key.long_name, row)    
                return tablename, None

            if len(parts) == 0:  
                if type == "map":
                    cmd = "map['{}']".format(key.id)
                    partition += self.execute_command(row, self.__map, None, cmd, key.long_name)
                    continue
                cell = self.get_value(key, row)            
                if type == "str" or type == "string":                    
                    partition += "{}p_{}={}".format(self.__sep,key.long_name,cell)
                    #delete the value used to generate the partition as we don't want the customer to query based on this value but rather we want them to query based on the partition attribute ie. p_event                
                    del row[key.id]                                           
                elif type == "datetime.datetime.utcfromtimestamp" and self.is_numeric(cell):                        
                    value = datetime.datetime.utcfromtimestamp(float(cell))
                    partition += "{}p_{}={}".format(self.__sep,key.long_name,value)
                elif type == "float" and self.is_numeric(cell):            
                    value = float(cell)
                    partition += "{}p_{}={}".format(self.__sep,key.long_name,value)
                elif (type == "int" or type == "integer") and self.is_numeric(cell):                        
                    value = int(cell)
                    partition += "{}p_{}={}".format(self.__sep,key.long_name,value)
                else:
                    continue            
            else:   
                cell = self.get_value(key, row)                      
                for part in parts:
                    if type == "str" or type == "string":
                        value = str(cell)
                        cmd = "value{}".format(part)
                    elif type == "datetime.datetime.utcfromtimestamp" and self.is_numeric(cell):                        
                        value = datetime.datetime.utcfromtimestamp(float(cell))
                        cmd = "value{}".format(part)
                    elif type == "float" and self.is_numeric(cell):            
                        value = float(cell)
                        cmd = "value{}".format(part)
                    elif (type == "int" or type == "integer") and self.is_numeric(cell):                        
                        value = int(cell)
                        cmd = "value{}".format(part)
                    else:
                        continue                    
                    partition += self.execute_command(row, self.__map, value, cmd, "{}_{}".format(key.long_name, part[1:]))
                
            if tablename is None:
                tablename = "table="+cell
        partition = '{0}{1}{2}'.format(self.__sep, tablename, partition )   
        return tablename, partition

    def execute_command(self, row, map, value, cmd, name):
        if "(" in name and name.index("(") >= 0:
            name = name[0:name.index("(")]
        try: 
            return "{}p_{}={}".format(self.__sep, name, eval(cmd))
        except NameError as e:
            print "Erroneous row:", row                        
            print "ERROR: Partitioner error: ", e
            return None              

    def get_value(self, key, row):
        if key.id in row:
            #short key name
            return str(row[key.id]).lower()                                      
        #long key name
        return str(row[key.long_name]).lower()            
    
    def is_date(self, val):
        try:
            float(val)
        except ValueError:
            try:
                complex(val)
            except ValueError:
                return False

        return True

    def is_numeric(self, val):
        if val.isdigit():
            return True

        try:
            float(val)
        except ValueError:
            try:
                complex(val)
            except ValueError:
                return False

        return True