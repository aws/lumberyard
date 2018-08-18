from io import BytesIO
from abc import ABCMeta, abstractmethod
import metric_constant as c
import os
import gzip
import zlib
import base64
import sqs
import enum_type

def checksum(data):
    return zlib.crc32(data)

def b64encode(data):
    return base64.b64encode(data)

def b64decode(data):
    return base64.b64decode(data)

COMPRESSION_MODE = enum_type.create(NONE="NoCompression", COMPRESS="Compress")

class CompressionClassFactory():
    @staticmethod
    def instance(name):
        #done this way for performance
        if name.lower() == COMPRESSION_MODE.NONE.lower():
            return NoCompression()
        if name.lower() == COMPRESSION_MODE.COMPRESS.lower():
            return Compress()
        

class AbstractCompression:
    __metaclass__ = ABCMeta
    @abstractmethod
    def compress(self, data, compressionlevel=None):
        raise NotImplementedError('subclasses must override compress')

    @abstractmethod
    def uncompress(self, data):
        raise NotImplementedError('subclasses must override uncompress')

    @abstractmethod
    def extract_message_body(self, message):
        raise NotImplementedError('subclasses must override extract_message_body') 
    
    @abstractmethod
    def add_message_payload(self, params, data):
        raise NotImplementedError('subclasses must override add_message_payload') 
    
    @property
    def identifier(self):        
        return self.__class__.__name__
    
    def size_of(self, data):
        return len(data)    

class Compress(AbstractCompression):
        
    def compress(self, data, compressionlevel=3):
        bytes = BytesIO()
        f = gzip.GzipFile(mode='wb',
                            compresslevel=compressionlevel,
                            fileobj=bytes)
        f.write(data)
        f.close()
        return bytes.getvalue()        

    def uncompress(self, data):
        return zlib.decompress(data, 16+15)

    def extract_message_body(self, message):
        return self.uncompress(message['MessageAttributes']['compressed_payload']['BinaryValue'])

    def add_message_payload(self, params, data):
        params["MessageBody"] = sqs.empty_body_message()
        params["MessageAttributes"]['compressed_payload'] = {
                        'BinaryValue': data,
                        'DataType': 'Binary'
                    }

class NoCompression(AbstractCompression):
    def compress(self, data):
        return data

    def uncompress(self, data):
        return data

    def extract_message_body(self, message):
        return message['Body']

    def add_message_payload(self, params, data):
        params["MessageBody"]= data
