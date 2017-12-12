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

import unittest
import mock

import properties

class TestString(unittest.TestCase):


    def test_with_string_and_no_default(self):
        
        target = properties.String()
        value = target('Test', 'value')

        self.assertEqual(value, 'value')


    def test_with_string_and_default(self):
        
        target = properties.String(default='Default')
        value = target('Test', 'value')

        self.assertEqual(value, 'value')


    def test_with_None_and_default(self):
        
        target = properties.String(default='value')
        value = target('Test', None)

        self.assertEqual(value, 'value')


    def test_with_None_and_no_default(self):
        
        target = properties.String()

        with self.assertRaises(properties.ValidationError) as e:
            value = target('Test', None)

        self.assertIn('Test', e.exception.message)


    def test_with_non_string(self):
        
        target = properties.String()

        with self.assertRaises(properties.ValidationError) as e:
            value = target('Test', 1234)

        self.assertIn('Test', e.exception.message)
        

class TestStringOrListOfString(unittest.TestCase):


    def test_with_string_and_no_default(self):
        
        target = properties.StringOrListOfString()
        value = target('Test', 'value')

        self.assertEqual(value, [ 'value' ])


    def test_with_string_and_default(self):
        
        target = properties.StringOrListOfString(default='Default')
        value = target('Test', 'value')

        self.assertEqual(value, [ 'value' ])


    def test_with_string_list(self):
        
        target = properties.StringOrListOfString()
        value = target('Test', [ 'value1', 'value2' ])

        self.assertEqual(value, [ 'value1', 'value2' ])


    def test_with_empty_list(self):
        
        target = properties.StringOrListOfString()
        value = target('Test', [ ])

        self.assertEqual(value, [ ])


    def test_with_None_and_default_string(self):
        
        target = properties.StringOrListOfString(default='value')
        value = target('Test', None)

        self.assertEqual(value, [ 'value' ])


    def test_with_None_and_default_list(self):
        
        target = properties.StringOrListOfString(default=[ 'value1', 'value2' ])
        value = target('Test', None)

        self.assertEqual(value, [ 'value1', 'value2' ])


    def test_with_None_and_no_default(self):
        
        target = properties.StringOrListOfString()

        with self.assertRaises(properties.ValidationError) as e:
            value = target('Test', None)

        self.assertIn('Test', e.exception.message)


    def test_with_not_string_or_list(self):
        
        target = properties.StringOrListOfString()

        with self.assertRaises(properties.ValidationError) as e:
            value = target('Test', 1234)

        self.assertIn('Test', e.exception.message)


    def test_with_non_string_in_list(self):
        
        target = properties.StringOrListOfString()

        with self.assertRaises(properties.ValidationError) as e:
            value = target('Test', [ 'value1', 1234 ])

        self.assertIn('Test', e.exception.message)



class TestObject(unittest.TestCase):


    def test_with_object_and_no_default(self):
        
        target = properties.Object(schema={'prop': properties.String()})
        value = target('Test', {'prop': 'value'})

        self.assertEqual(value.prop, 'value')


    def test_with_object_and_default(self):
        
        target = properties.Object(default={'prop': 'default'}, schema={'prop': properties.String()})
        value = target('Test', {'prop': 'value'})

        self.assertEqual(value.prop, 'value')


    def test_with_None_and_default(self):
        
        target = properties.Object(default={'prop': 'default'}, schema={'prop': properties.String()})
        value = target('Test', None)

        self.assertEqual(value.prop, 'default')


    def test_with_None_and_no_default(self):
        
        target = properties.Object(schema={'prop': properties.String()})

        with self.assertRaises(properties.ValidationError) as e:
            value = target('Test', None)

        self.assertIn('Test', e.exception.message)


    def test_with_non_object(self):
        
        target = properties.Object(schema={'prop': properties.String()})

        with self.assertRaises(properties.ValidationError) as e:
            value = target('Test', 1234)

        self.assertIn('Test', e.exception.message)


class TestObjectOrListOfObject(unittest.TestCase):


    def test_with_object_and_no_default(self):
        
        target = properties.ObjectOrListOfObject(schema={'prop': properties.String()})
        value = target('Test', {'prop': 'value'})

        self.assertEqual(len(value), 1)
        self.assertEqual(value[0].prop, 'value')


    def test_with_object_and_default(self):
        
        target = properties.ObjectOrListOfObject(schema={'prop': properties.String()}, default={'prop': 'default'})
        value = target('Test', {'prop': 'value'})

        self.assertEqual(len(value), 1)
        self.assertEqual(value[0].prop, 'value')


    def test_with_object_list(self):
        
        target = properties.ObjectOrListOfObject(schema={'prop': properties.String()})
        value = target('Test', [ {'prop': 'value1'}, {'prop': 'value2'} ])

        self.assertEqual(len(value), 2)
        self.assertEqual(value[0].prop, 'value1')
        self.assertEqual(value[1].prop, 'value2')


    def test_with_empty_list(self):
        
        target = properties.ObjectOrListOfObject(schema={'prop': properties.String()})
        value = target('Test', [ ])

        self.assertEqual(len(value), 0)


    def test_with_None_and_default_object(self):
        
        target = properties.ObjectOrListOfObject(schema={'prop': properties.String()}, default={'prop': 'value'})
        value = target('Test', None)

        self.assertEqual(len(value), 1)
        self.assertEqual(value[0].prop, 'value')


    def test_with_None_and_default_list(self):
        
        target = properties.ObjectOrListOfObject(schema={'prop': properties.String()}, default=[ {'prop': 'value1'}, {'prop': 'value2'} ])
        value = target('Test', None)

        self.assertEqual(len(value), 2)
        self.assertEqual(value[0].prop, 'value1')
        self.assertEqual(value[1].prop, 'value2')


    def test_with_None_and_no_default(self):
        
        target = properties.ObjectOrListOfObject(schema={'prop': properties.String()})

        with self.assertRaises(properties.ValidationError) as e:
            value = target('Test', None)

        self.assertIn('Test', e.exception.message)


    def test_with_not_object_or_list(self):
        
        target = properties.ObjectOrListOfObject(schema={'prop': properties.String()})

        with self.assertRaises(properties.ValidationError) as e:
            value = target('Test', 1234)

        self.assertIn('Test', e.exception.message)


    def test_with_non_object_in_list(self):
        
        target = properties.ObjectOrListOfObject(schema={'prop': properties.String()})

        with self.assertRaises(properties.ValidationError) as e:
            value = target('Test', [ {'prop': 'value' }, 1234 ])

        self.assertIn('Test', e.exception.message)


    def test_with_invalid_object_in_list(self):
        
        target = properties.ObjectOrListOfObject(schema={'prop': properties.String()})

        with self.assertRaises(properties.ValidationError) as e:
            value = target('Test', [ {'prop': 1234 } ])

        self.assertIn('Test', e.exception.message)


class TestProperties(unittest.TestCase):


    def test_calls_validator_with_provided_value(self):

        event = {
            'ResourceProperties': {
                'Test': 'Value'
            }
        }

        def handler(name, value):
            self.assertEqual(name, 'Test')
            self.assertEqual(value, 'Value')
            return 'Result'

        schema = {
            'Test': handler
        }

        props = properties.load(event, schema)

        self.assertEqual('Result', props.Test)


    def test_calls_validator_when_no_provided_value(self):

        event = {
            'ResourceProperties': {
            }
        }

        def handler(name, value):
            self.assertEqual(name, 'Test')
            self.assertEqual(value, None)
            return 'Result'

        schema = {
            'Test': handler
        }

        props = properties.load(event, schema)

        self.assertEqual('Result', props.Test)


    def test_detects_property_not_in_schema(self):

        event = {
            'ResourceProperties': {
                'Test': 'Value'
            }
        }

        schema = {}

        with self.assertRaises(properties.ValidationError) as e:
            properties.load(event, schema)

        self.assertIn('Test', e.exception.message)


    def test_calls_validator_with_provided_value_when_wildcard(self):

        event = {
            'ResourceProperties': {
                'Test1': 'Value1',
                'Test2': 'Value2'
            }
        }

        def handler(name, value):
            if name == 'Test1':
                self.assertEqual(value, 'Value1')
                return 'Result1'
            if name == 'Test2':
                self.assertEqual(value, 'Value2')
                return 'Result2'
            self.assertTrue(False)

        schema = {
            '*': handler
        }

        props = properties.load(event, schema)

        self.assertEqual('Result1', props.Test1)
        self.assertEqual('Result2', props.Test2)


    def test_calls_correct_validator_with_provided_value_when_wildcard(self):

        event = {
            'ResourceProperties': {
                'Test1': 'Value1',
                'Test2': 'Value2'
            }
        }

        def handler1(name, value):
            self.assertEqual(name, 'Test1')
            self.assertEqual(value, 'Value1')
            return 'Result1'

        def handler2(name, value):
            self.assertEqual(name, 'Test2')
            self.assertEqual(value, 'Value2')
            return 'Result2'

        schema = {
            '*': handler1,
            'Test2': handler2
        }

        props = properties.load(event, schema)

        self.assertEqual('Result1', props.Test1)
        self.assertEqual('Result2', props.Test2)


class TestInteger(unittest.TestCase):


    def test_with_int_and_no_default(self):
        
        target = properties.Integer()
        value = target('Test', 10)

        self.assertEqual(value, 10)


    def test_with_int_and_default(self):
        
        target = properties.Integer(default=42)
        value = target('Test', 10)

        self.assertEqual(value, 10)


    def test_with_None_and_default(self):
        
        target = properties.Integer(default=42)
        value = target('Test', None)

        self.assertEqual(value, 42)


    def test_with_None_and_no_default(self):
        
        target = properties.Integer()

        with self.assertRaises(properties.ValidationError) as e:
            value = target('Test', None)

        self.assertIn('Test', e.exception.message)


    def test_with_non_int(self):
        
        target = properties.Integer()

        with self.assertRaises(properties.ValidationError) as e:
            value = target('Test', 'string')

        self.assertIn('Test', e.exception.message)
        

class TestBoolean(unittest.TestCase):


    def test_with_bool_and_no_default(self):
        
        target = properties.Boolean()
        value = target('Test', True)

        self.assertEqual(value, True)


    def test_with_bool_and_default(self):
        
        target = properties.Boolean(default=False)
        value = target('Test', True)

        self.assertEqual(value, True)


    def test_with_None_and_default(self):
        
        target = properties.Boolean(default=False)
        value = target('Test', None)

        self.assertEqual(value, False)


    def test_with_None_and_no_default(self):
        
        target = properties.Boolean()

        with self.assertRaises(properties.ValidationError) as e:
            value = target('Test', None)

        self.assertIn('Test', e.exception.message)


    def test_with_bool_string(self):
        target = properties.Boolean()
        self.assertTrue(target('Test', 'true'))
        self.assertTrue(target('Test', 'True'))
        self.assertTrue(target('Test', 'TRUE'))
        self.assertFalse(target('Test', 'false'))
        self.assertFalse(target('Test', 'False'))
        self.assertFalse(target('Test', 'FALSE'))


    def test_with_non_bool(self):
        
        target = properties.Boolean()

        with self.assertRaises(properties.ValidationError) as e:
            value = target('Test', 'string')

        self.assertIn('Test', e.exception.message)
        

class TestDictionary(unittest.TestCase):


    def test_with_dict_and_no_default(self):
        
        target = properties.Dictionary()
        value = target('Test', { 'a': 1 })

        self.assertEqual(value, { 'a': 1 })


    def test_with_dict_and_default(self):
        
        target = properties.Dictionary(default={ 'a': 2 })
        value = target('Test', { 'a': 1 })

        self.assertEqual(value, { 'a': 1 })


    def test_with_None_and_default(self):
        
        target = properties.Dictionary(default={ 'a': 2 })
        value = target('Test', None)

        self.assertEqual(value, { 'a': 2 })


    def test_with_None_and_no_default(self):
        
        target = properties.Dictionary()

        with self.assertRaises(properties.ValidationError) as e:
            value = target('Test', None)

        self.assertIn('Test', e.exception.message)


    def test_with_non_dict(self):
        
        target = properties.Dictionary()

        with self.assertRaises(properties.ValidationError) as e:
            value = target('Test', 'string')

        self.assertIn('Test', e.exception.message)
        


