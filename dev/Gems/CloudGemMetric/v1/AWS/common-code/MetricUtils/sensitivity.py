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

SENSITIVITY_TYPE = enum_type.create(NONE="Insensitive", ENCRYPT="Sensitive")

class SensitivityClassFactory():
    @staticmethod
    def instance(name):
        #done this way for performance
        if name.lower() == SENSITIVITY_TYPE.NONE.lower():
            return Unencrypted()
        if name.lower() == SENSITIVITY_TYPE.ENCRYPT.lower():
            return Encrypted()
        

class AbstractSensitivity:
    __metaclass__ = ABCMeta

class Unencrypted(AbstractSensitivity):        
    def __init__(self):
        pass
    
    @property
    def identifier(self):
        return SENSITIVITY_TYPE.NONE

"""
Encrypt S3 files (0) or leave unencrypted (1)
"""
class Encrypted(AbstractSensitivity):
    def __init__(self):
        pass
    
    @property
    def identifier(self):
        return SENSITIVITY_TYPE.ENCRYPT