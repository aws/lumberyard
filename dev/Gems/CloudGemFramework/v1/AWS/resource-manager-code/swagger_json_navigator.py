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

REQUIRED = {} # special value used when no default is acceptable

class SwaggerNavigator(object):

    def __init__(self, value, parent = None, selector=None):
        self.value = value
        self.parent = parent
        self.selector = selector

    def __repr__(self):
        if not self.parent:
            return "swagger.json:"
        return self.parent.__repr__() + "[{}]".format(self.formatted_selector)

    @property
    def root(self):
        if self.parent:
            return self.parent.root
        else:
            return self

    @property
    def formatted_selector(self):
        if isinstance(self.selector, str):
            return "\"{}\"".format(self.selector)
        return self.selector

    def contains(self, value):
        if self.is_object or self.is_array:
            return value in self.value
        else:
            raise ValueError('{} is not an object or array.'.format(self))

    def get(self, selector, default=REQUIRED):
        if self.is_object:
            if selector not in self.value:
                if default is REQUIRED:
                    raise ValueError('{} has no {} property.'.format(self, selector))
                else:
                    found_value = default
            else:
                found_value = self.value[selector]
            return SwaggerNavigator(found_value, parent=self, selector = selector)

        elif self.is_array:
            if not isinstance(selector, int):
                raise ValueError('{} is a list but accessor {} is not an integer.'.format(self, selector))

            if selector < 0 or selector >= len(self.value):
                if default is REQUIRED:
                    raise ValueError('{} has no index {}.'.format(self, selector))
                else:
                    found_value = default
            else:
                found_value = self.value[selector]
            return SwaggerNavigator(found_value, parent=self, selector = selector)

        raise ValueError('{} is not an object or array. Cannot select {}.'.format(self, selector))

    def remove(self, selector, required=False):
        if self.is_object:
            if selector in self.value:
                del self.value[selector]
            elif required:
                raise ValueError('{} does not contain {} so it cannot be removed.'.format(self, selector))
        elif self.is_array:
            if selector >= 0 and selector < len(self.value):
                del self.value[selector]
            elif required:
                raise ValueError('{} does not contain {} so it cannot be removed.'.format(self, selector))
        else:
            raise ValueError('{} is not an object or array. Cannot remove {}.'.format(self, selector))

    def values(self):
        if self.is_object:
            return [self.get(key) for key in self.value.keys()]
        elif self.is_array:
            return [self.get(i) for i in range(len(self.value))]
        return None

    def items(self):
        if self.is_object:
            return [(key, self.get(key)) for key in self.value.keys()]
        elif self.is_array:
            return [(i, self.get(i)) for i in range(len(self.value))]
        return None

    @property
    def is_none(self):
        return self.value == None

    @property
    def is_empty(self):
        if self.is_none:
            return True
        elif self.is_object or self.is_array: 
            return len(self.value) == 0
        else:
            return False

    @property
    def is_object(self):
        return isinstance(self.value, dict)

    def get_object(self, selector, default=REQUIRED):

        navigator = self.get(selector, default=default)
        if not (navigator.is_object or navigator.is_none):
            raise ValueError('{} value {} is not an object.'.format(navigator, navigator.value))

        return navigator

    def get_object_value(self, selector, default=REQUIRED):
        navigator = self.get_object(selector, default)
        return navigator.value

    def get_or_add_object(self, selector, default=None):
        if not self.contains(selector):
            if self.is_object:
                self.value[selector] = default if default else {}
            elif self.is_array:
                self.value.insert(selector, default if default else {})
            else:
                raise ValueError('{} is not an object or array. Cannot add {} at {}.'.format(self, default, selector))        
        return self.get_object(selector)

    def add_object(self, selector, initial_value):
        
        if self.contains(selector):
            raise ValueError('{} already contains an {} value.'.format(self, selector))

        if self.is_object:
            self.value[selector] = initial_value
        elif self.is_array:
            self.value.insert(selector, initial_value)
        else:
            raise ValueError('{} is not an object or array. Cannot add {} at {}.'.format(self, initial_value, selector))        

        return self.get_object(selector)

    def remove_object(self, selector, default=REQUIRED):
        navigator = self.get_object(selector, default)
        self.remove(selector, default is REQUIRED)
        return navigator

    @property
    def is_array(self):
        return isinstance(self.value, list)

    def get_array(self, selector, default=REQUIRED, auto_promote_singletons = False):

        navigator = self.get(selector, default=default)
        if not (navigator.is_array or navigator.is_none):
            if auto_promote_singletons:
                self.value[selector] = [navigator.value]
                navigator = self.get(selector)
            else:
                raise ValueError('{} value {} is not an array.'.format(navigator, navigator.value))

        return navigator

    def get_or_add_array(self, selector, default=None):
        if not self.contains(selector):
            if self.is_object:
                self.value[selector] = default if default else []
            elif self.is_array:
                self.value.insert(selector, default if default else [])
            else:
                raise ValueError('{} is not an object or array. Cannot add {} at {}.'.format(self, default, selector))        
        return self.get_array(selector)

    def get_array_value(self, selector, default=REQUIRED):
        navigator = self.get_array(selector, default)
        return navigator.value

    def remove_array(self, selector, default=REQUIRED):
        navigator = self.get_array(selector, default)
        self.remove(selector, default is REQUIRED)
        return navigator

    @property
    def is_string(self):
        return isinstance(self.value, str) or isinstance(self.value, unicode)

    def get_string(self, selector, default=REQUIRED):

        navigator = self.get(selector, default=default)
        if not (navigator.is_string or navigator.is_none):
            raise ValueError('{} value {} is not a string.'.format(navigator, navigator.value))

        return navigator

    def get_string_value(self, selector, default=REQUIRED):
        navigator = self.get_string(selector, default)
        return navigator.value

    def remove_string(self, selector, default=REQUIRED):
        navigator = self.get_string(selector, default)
        self.remove(selector, default is REQUIRED)
        return navigator

    @property
    def is_boolean(self):
        return isinstance(self.value, bool)

    def get_boolean(self, selector, default=REQUIRED):

        navigator = self.get(selector, default=default)
        if not (navigator.is_boolean or navigator.is_none):
            raise ValueError('{} value {} is not a boolean.'.format(navigator, navigator.value))

        return navigator

    def get_boolean_value(self, selector, default=REQUIRED):
        navigator = self.get_boolean(selector, default)
        return navigator.value

    def remove_boolean(self, selector, default=REQUIRED):
        navigator = self.get_boolean(selector, default)
        self.remove(selector, default is REQUIRED)
        return navigator

    @property
    def is_int(self):
        return isinstance(self.value, int)

    def get_int(self, selector, default=REQUIRED):

        navigator = self.get(selector, default=default)
        if not (navigator.is_int or navigator.is_none):
            raise ValueError('{} value {} is not a string.'.format(navigator, navigator.value))

        return navigator

    def get_int_value(self, selector, default=REQUIRED):
        navigator = self.get_int(selector, default)
        return navigator.int

    def remove_int(self, selector, default=REQUIRED):
        navigator = self.get_int(selector, default)
        self.remove(selector, default is REQUIRED)
        return navigator

    def get_root(self):
        if self.parent == None:
            return self
        return self.parent.get_root()

    @property
    def is_ref(self):
        return self.is_object and self.contains('$ref')

    # Returns a SwaggerNavigator object at the position of the ref this navigator has found
    def resolve_ref(self, ref = None):

        if not ref: ref = self.get_string('$ref').value

        ref_path = ref.split("/")
        if ref_path[0] == "#":
            nav = self.get_root()
            ref_path.pop(0)
        else:
            nav = self.parent

        while ref_path:
            try:
                nav = nav.get(ref_path.pop(0))
            except ValueError as e:
                raise ValueError("Error {} while resolving reference {} at {}".format(e, ref, self))

        return nav
