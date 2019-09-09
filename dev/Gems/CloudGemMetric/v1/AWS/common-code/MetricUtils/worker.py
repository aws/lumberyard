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
            output = None
            try:
                func, args, kargs = self.tasks.get()
                output = func(*args, **kargs)
            except Exception as e:            
                self.debug_thread(func, args, kargs, output)
                print(traceback.format_exc())  
                print e              
            finally:            
                self.tasks.task_done()

    #Requires verbose CLI flag
    def debug_thread(self, func, args, kargs, output):
        util.debug_print("Function called by thread: \n")
        util.debug_print(func)
        util.debug_print("Function args: \n")
        util.debug_print(args)
        util.debug_print("Function kargs: \n")            
        util.debug_print(kargs)
        util.debug_print("Function output: \n")
        util.debug_print(output)