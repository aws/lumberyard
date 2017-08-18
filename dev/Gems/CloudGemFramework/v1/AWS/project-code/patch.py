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

class OperationListBuilder(object):
    '''Simplifies the building of a patch operation list for use with API Gateway.
    See http://jsonpatch.com/ for more information, but API Gateway doesn't support
    everything described there.'''

    OP_ADD     = 'replace' # API Gateway seems to require 'replace' even for paths that don't exist
    OP_REPLACE = 'replace'
    OP_REMOVE  = 'remove'

    VALUE_TYPES = (basestring, int, bool)

    def __init__(self):
        self.__list = []

    def get(self):
        return self.__list

    def replace(self, path, value):
        if not isinstance(value, self.VALUE_TYPES):
            raise ValueError('Type {} not supported when constructing patch operations for path {} property {} with value {}.'.format(type(value), path, key, value))
        self.__list.append({ 'op': self.OP_REPLACE, 'path': path, 'value': value })

    def remove(self, path):
        self.__list.append({ 'op': self.OP_REMOVE, 'path': path })

    def add(self, path, value):
        if not isinstance(value, self.VALUE_TYPES):
            raise ValueError('Type {} not supported when constructing patch operations for path {} property {} with value {}.'.format(type(value), path, key, value))
        self.__list.append({ 'op': self.OP_ADD, 'path': path, 'value': value })

    def diff(self, path, old, new):

        for key, new_value in new.iteritems():
            if key not in old:
                if isinstance(new_value, dict):
                    # new dict
                    self.diff(path + key + '/', {}, new_value)
                else:
                    # new single
                    self.add(path + key, new_value)
            else: # key in old
                old_value = old[key]
                if isinstance(new_value, dict) and isinstance(old_value, dict):
                    # both dicts
                    self.diff(path + key + '/', old_value, new_value)
                elif isinstance(new_value, dict):
                    # new dict, old single
                    self.remove(path + key)
                    self.diff(path + key + '/', {}, new_value)
                elif isinstance(old_value, dict):
                    # old dict, new single
                    self.diff(path + key + '/', old_value, {})
                    self.add(path + key, new_value)
                else:
                    # new single, old single
                    if old_value != new_value:
                        self.replace(path + key, new_value)

        for key, old_value in old.iteritems():
            if key not in new:
                if isinstance(old_value, dict):
                    self.diff(path + key + '/', old_value, {})
                else:
                    self.remove(path + key)

