import time
import metric_constant as c
import os
import psutil

def get_memory_usage():   
    memory = psutil.virtual_memory()        
    return memory.percent

def get_process_memory_usage_bytes():
    pid = os.getpid()
    py = psutil.Process(pid)
    memoryUseBytes = py.memory_info()[0]/2        
    return memoryUseBytes

def get_process_memory_usage_kilobytes():           
    return get_process_memory_usage_bytes()/1024

def get_process_memory_usage_megabytes():       
    return get_process_memory_usage_kilobytes()/1024

def get_memory_object():
    return psutil.virtual_memory()  