from Queue import Queue
from threading import Thread
from worker import Worker

"""
Inspired by Chris Hager's work
https://www.metachris.com/2016/04/python-threadpool/
"""
class ThreadPool:    
    def __init__(self, context={}, size=2):
        self.tasks = Queue(size)
        for num in range(size):
            Worker(self.tasks, context)

    def add(self, func, *args, **kargs):
        self.tasks.put((func, args, kargs))

    def wait(self):        
        self.tasks.join()
    
