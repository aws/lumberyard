import metric_constant as c
import traceback
import sys
from threading import Thread
import util

"""
Inspired by Chris Hager's work
https://www.metachris.com/2016/04/python-threadpool/
"""
class Worker(Thread):
    def __init__(self, tasks, context):
        Thread.__init__(self)
        self.tasks = tasks
        self.daemon = True
        self.start()                

    def run(self):      
        while True:  
            try:
                func, args, kargs = self.tasks.get()            
                util.debug_print("Function called by thread: \n")
                util.debug_print(func)
                util.debug_print("Function args: \n")
                util.debug_print(args)
                util.debug_print("Function kargs: \n")            
                util.debug_print(kargs)
                util.debug_print("Function output: \n")
                util.debug_print(func(*args, **kargs))
            except Exception as e:            
                print(traceback.format_exc())
                raise e
            finally:            
                self.tasks.task_done()
