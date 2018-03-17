#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# $Revision: #4 $

class Data(object):
    '''Provides access to a dict using attributes.

    Each key in the dict corresponds to an attribute on the object. To
    get a dict entry, simply do 'value = data.Foo', where Foo is the key.
    To create or change a dict entry, do 'data.Foo = value'.

    When a dict typed property value is returned, it is wrapped by a 
    Data object.

    When a list typed property is returned, a copy of the origional list  
    is returned with any dict typed items replaced by Data wrappers. This 
    is done recursively through nested lists.

    Getting the same attribute's value multiple times will result in the same
    object instance (wrapped, converted, or origional) being returned every 
    time.

    You can prevent the accidential creation of entries by setting the 
    read_only parameter to True when initializing the object.

    You can control the type of auto-created attribute values using the
    create_new paramter when initializing the object. By default new dict
    objects are created.

    All the keys in the dict must be valid Python attribute names. To get a
    dict that is known to contain invalid keys, use x.DATA['Foo'] instead of 
    x.Foo.
    '''

    def __init__(self, data = None, read_only = False, create_new = dict, **kwargs):
        '''Initialize data content.

        Arguments:

          - data: the wrapped dict. Defaults to a new empty dict.

          - read_only: True will prevent the automatic creation of new 
          attributes. Instead a RuntimeError is raised if a requested
          attribute doesn't have a corresponding key in the dict used to
          initialize the object. 
          
          Note that this does not prevent the data from being directly 
          manipulated via the DATA property. Defaults to False.

          - create_new: A function called to create new attribute values.
          By default dict objects are created (so you can a.b.c to create
          nested data). 
          
          A value of None will cause all new attributes to have the value 
          None, which can be useful as an alturnative to setting read_only 
          to True and doing "if 'foo' in data" all the time. Instead you
          can do "if data.foo" (or "if data.foo is not None").

          - **kwargs: Specifies additional 'metadata' attributes to be set
          on the Data object. These will not be included in the dict. By
          convention, these the names should be all upper cases.

        '''

        if data is None:
            data = {}

        if not isinstance(data, dict):
            raise ValueError('Initial Data data is not a dict, instead found {}'.format(type(data)))

        self.__setattr_super('_Data__data', data)
        self.__setattr_super('_Data__read_only', read_only)
        self.__setattr_super('_Data__create_new', create_new)

        for k, v in data.iteritems():
            self.__setattr_adapt(k, v)

        for k, v in kwargs.iteritems():
            self.__setattr_super(k, v)

    @property
    def DATA(self):
        '''Get the current data.'''
        return self.__data

    @property
    def READ_ONLY(self):
        '''Determine if read_only was True when the object was created.'''
        return self.__read_only


    def __contains__(self, item):
        '''Determine if an attribute exists on this object.'''
        return item in self.__data 


    def __eq__(self, other):
        '''Called by Python when comparing this object to another object.'''
        return (isinstance(other, Data) and self.__data == other.__data) \
            or (isinstance(other, dict) and self.__data == other)


    def __str__(self):
        '''Called by Python when converting this object to a string.'''
        return self.__data.__str__()


    def __repr__(self):
        '''Called by Python to create a string representation of this object.'''
        return 'Data({})'.format(str(self))


    def __getattr__(self, name):
        '''Called by Python to retreive the value of undefined attribute.'''

        if name.startswith('__'):
            return super(Data, self).__getattr__(name) # use default for built in attributes
        else:

            if self.__read_only:
                raise RuntimeError('Data object is read only when attempting get non-existant property {}.'.format(name))

            if self.__create_new:
                new_value = self.__create_new() 
            else:
                new_value = None

            return self.__setattr_adapt(name, new_value)


    def __setattr__(self, name, value):
        '''Called by Python when an attribute is set on this object.'''

        if self.__read_only:
            raise RuntimeError('Data object is read only when attempting to set property {} to value {}.'.format(name, value))

        self.__data[name] = value
        self.__setattr_adapt(name, value)


    def __setattr_adapt(self, name, value):
        '''Set an attribute, using a wrapper or conversion as needed.'''

        if isinstance(value, dict):
            value = Data(value)
        elif isinstance(value, list):
            value = self.__convert_list(value)

        self.__setattr_super(name, value)

        return value


    def __setattr_super(self, name, value):
        '''Set an attribute without triggering our __setattr__ override.'''
        super(Data, self).__setattr__(name, value)


    def __convert_list(self, source):
        '''Recursivly wraps dict objects in a list.'''
        target = []
        for i in source:
            if isinstance(i, dict):
                i = Data(i)
            elif isinstance(i, list):
                i = self.__convert_list(i)
            target.append(i)
        return target

    def __nonzero__(self):
       return len(self.__data) != 0
  
    def __len__(self):
        return len(self.__data)
