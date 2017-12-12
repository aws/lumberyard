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
# $Revision: #1 $

from types import *
from errors import ValidationError

class _Properties(object):

    def __init__(self, src, schema, prefix=''):

        wildcard = None
        for name, validator in schema.iteritems():
            if name == '*':
                wildcard = validator
            else:
                setattr(self, name, validator(prefix + name, src.get(name, None)))

        for name, value in src.iteritems():
            if name not in schema and name != 'ServiceToken':
                if wildcard:
                    setattr(self, name, wildcard(prefix + name, value))
                else:
                    raise ValidationError('The {} property is not supported.'.format(name))


class String(object):

    def __init__(self, default = None):
        self.default = default

    def __call__(self, name, value):
        
        if value is None:
            value = self.default

        if value is None:
            raise ValidationError('A value for property {} must be provided.'.format(name))

        if not isinstance(value, basestring):
            raise ValidationError('The {} property value must be a string.'.format(name))

        return value


class StringOrListOfString(object):

    def __init__(self, default = None, minOccurs = None, maxOccurs = None):
        self.default = default

    def __call__(self, name, value):

        if value is None:
            value = self.default

        if value is None:
            raise ValidationError('A value for property {} must be provided.'.format(name))

        if isinstance(value, basestring):

            return [ value ]

        else:

            if not isinstance(value, list):
                raise ValidationError('The {} property value must be a string or a list of strings.'.format(name))

            for entry in value:
                if not isinstance(entry, basestring):
                    raise ValidationError('The {} property must be a string or a list of strings.'.format(name))

            return value


class Object(object): 

    def __init__(self, default = None, schema = None):
        self.default = default
        self.schema = schema if schema else {}

    def __call__(self, name, value):

        if value is None:
            value = self.default

        if value is None:
            raise ValidationError('A value for property {} must be provided.'.format(name))

        if not isinstance(value, dict):
            raise ValidationError('The {} property value must be a dictionary.'.format(name))

        return _Properties(value, self.schema, prefix=name + '.')


class ObjectOrListOfObject(object):

    def __init__(self, default = None, schema = None):
        self.default = default
        self.Entry = Object(schema=schema)

    def __call__(self, name, value):

        if value is None:
            value = self.default

        if value is None:
            raise ValidationError('A value for property {} must be provided.'.format(name))

        if not isinstance(value, list):
            value = [ value ]

        result = []
        for index, entry in enumerate(value):
            result.append(self.Entry('{}[{}]'.format(name, index), entry))

        return result


class Dictionary(object):

    def __init__(self, default = None):
        self.default = default

    def __call__(self, name, value):

        if value is None:
            value = self.default

        if value is None:
            raise ValidationError('A value for property {} must be provided.'.format(name))

        if not isinstance(value, dict):
            raise ValidationError('The {} property value must be a dictionary.'.format(name))

        return value


class Boolean(object):

    def __init__(self, default = None):
        self.default = default

    def __call__(self, name, value):
        
        if value is None:
            value = self.default

        if value is None:
            raise ValidationError('A value for property {} must be provided.'.format(name))

        # Cloud Formation doesn't support Boolean typed parameters. Check for
        # boolean strings so that string parameters can be used to initialize 
        # boolean properties.
        if isinstance(value, basestring):
            lower_value = value.lower()
            if lower_value == 'true':
                value = True
            elif lower_value == 'false':
                value = False

        if not isinstance(value, bool):
            raise ValidationError('The {} property value must be boolean.'.format(name))

        return value


class Integer(object):

    def __init__(self, default = None):
        self.default = default

    def __call__(self, name, value):
        
        if value is None:
            value = self.default

        if value is None:
            raise ValidationError('A value for property {} must be provided.'.format(name))

        if not isinstance(value, int):
            raise ValidationError('The {} property value must be an integer.'.format(name))

        return value


def load(event, schema):
    return _Properties(event['ResourceProperties'], schema)

def load_old(event, schema):
    return _Properties(event['OldResourceProperties'], schema)
