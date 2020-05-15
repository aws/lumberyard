#
# testing Python import package
# 
import sys

def test_call():
    print ('test_call_hit')

class TestType:
   def __init__(self, *args, **kwargs):
       return super().__init__(*args, **kwargs)
   def do_call(self, value):
       print ('TestType.do_call.{}'.format(value))
