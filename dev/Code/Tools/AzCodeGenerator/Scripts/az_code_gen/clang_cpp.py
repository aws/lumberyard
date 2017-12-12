# A series of python functions for parsing
# JSON data packed into a string

#################
# Parsing Driver
#################
# expand_annotations(Dict)

####################
# Parsing Functions
####################
# build_tree_from_string(string)
# is_simple_token(string, string)
# is_parametrized_token(string, string)
# extract_tag(string, string)
# extract_parameters(string)
# split_on_major_delimiter(string, string)

# For more information, see the header comments on each
import re
from collections import defaultdict, OrderedDict

# regex to match (Something) -> {tag: Something} or (Something<T, U>) -> {tag: Something, template_params: [T, U]}
TEMPLATE_TAG_PATTERN = re.compile(r'(?P<tag>[\w\d_:]+)(\<(?P<template_params>[\w\d\s_:,]+)\>)?')
# split a string of the form A::B::C or A.B.C into (A, B, C)
TAG_SPLIT_PATTERN = re.compile(r'[:\.]+')

def resolve_annotation(annotations, tag, default=None):
    """ Resolves an annotation in C++ namespace format to its value or None
        e.g. AzClass::Serialize::Base -> annotations['AzClass']['Serialize']['Base']
    @param annotations The root annotation dict to search through
    @param tag The C++ formatted tag (using either :: or . as a separator)
    @param default The value to return if no annotation is found (default: None)
    @return The resolved annotation value or default 
    """
    tag_parts = re.split(TAG_SPLIT_PATTERN, tag)
    try:
        namespace = annotations
        for part in tag_parts[0:-1]:
            namespace = namespace.get(part, {})
        return namespace.get(tag_parts[-1], default)
    except:
        pass
    return default

def store_annotation(annotations, tag, value, overwrite=False):
    """ Adds an annotation specified in C++ namespace format to an annotation dictionary
        using the namespaces to specify dict hierarchy
    @param annotations The root annotation dict to store into
    @param tag The C++ formatted tag (using either :: or . as a separator)
    @param value The value to store
    """
    tag_parts = re.split(TAG_SPLIT_PATTERN, tag)
    namespace = annotations
    for part in tag_parts[0:-1]:
        # if there is a None value already in place, replace it with a table
        # this happens in cases like Serialize() and Serialize::Base<>()
        if namespace.has_key(part) and not isinstance(namespace[part], dict):
            del namespace[part]
        namespace = namespace.setdefault(part, {})
    # do not stomp a parent table with a value
    if overwrite or \
        not namespace.has_key(tag_parts[-1]) or \
        not isinstance(namespace[tag_parts[-1]], dict):
        namespace[tag_parts[-1]] = value

def format_cpp_annotations(json_object):
    """ Does a pass over the entire JSON output from clang and organizes it into:
        {
            "objects": [
                {
                    "fields": [
                        {
                            "annotations": {
                                "AzProperty": {}
                            }
                        }
                    ],
                    "methods": [],
                    "annotations": {
                        "AzClass": {}
                        "AzComponent": {}
                    }
                }
            ],
            "enums": [
                {
                    "annotations": {
                        "AzEnum": {}
                    }
                }
            ]
        }
        This includes resolving annotations that are attached to special codegen virtual variables
        and generally cleaning up the top-level structure and annotation hierarchy of the input JSON
    @param json_object The output from the clang AST parse/dump
    """
    promote_field_tags_to_class(json_object)
    format_enums(json_object)
    format_globals(json_object)

    for object in json_object.get('objects', []):
        # convert tokens -> nested dicts
        for field in object.get('fields', []):
            expand_annotations( field )
        for method in object.get('methods', []):
            expand_annotations( method )
        expand_annotations(object)
    for enum in json_object.get('enums', []):
        expand_annotations(enum)
        coalesce_enum_values(enum)


def format_enums(json_object):
    """ Collects all enums and enum annotations found in json_object['objects'] and
        coalesces/moves them into json_object['enums']
    @param json_object The raw JSON output from clang's AST dump
    """
    enums = {}
    annotations = {}
    decls_to_remove = set()
    for object in json_object.get('objects', []): 
        # objects with type 'enum SomeEnum' are enum annotations
        cpp_type = object['type']
        if 'canonical_type' in object:
            cpp_type = object['canonical_type']
        if cpp_type.startswith('enum '):
            enum = cpp_type.replace('enum ', '')
            annotations[enum] = object['annotations']
            decls_to_remove.add(object['qualified_name'])
        # objects with type enum are the enums themselves
        elif cpp_type == 'enum':
            enum = object['qualified_name']
            enums[enum] = object
            decls_to_remove.add(object['qualified_name'])

    # move enums into the top level enums entry and coalesce annotations into them
    json_object['enums'] = []
    for name, enum in enums.iteritems():
        if name in annotations: # only include enums with annotations
            enum_annotations = annotations[name]
            enum.setdefault('annotations', {}).update(enum_annotations)
            json_object['enums'].append(enum)

    # Strip out top level decls that have been coalesced
    if len(decls_to_remove) > 0:
        json_object['objects'] = [obj for obj in json_object['objects'] if obj['qualified_name'] not in decls_to_remove]

def coalesce_enum_values(enum):
    """ Condense {AzEnum::Values: {AzEnum::Value: []}} to {AzEnum::Values: []}
    """
    values = resolve_annotation(enum['annotations'], 'AzEnum::Values', {})
    values = values.get('AzEnum::Value', [])
    if values:
        store_annotation(enum['annotations'], 'AzEnum::Values', values)

def format_globals(json_object):
    decls_to_remove = set()
    globals = []
    for decl in json_object.get('objects', []):
        if decl['type'] not in ('class', 'struct', 'enum'):
            globals.append(decl)
            decls_to_remove.add(decl['qualified_name'])

    json_object['globals'] = globals

    # Strip out top level decls that have been coalesced
    if len(decls_to_remove) > 0:
        json_object['objects'] = [obj for obj in json_object['objects'] if obj['qualified_name'] not in decls_to_remove]

def promote_field_tags_to_class(json_object):
    """ Promote field annotations to the class object if they contain the "class_attribute" attribute
    """
    remove_virtual_fields = True
    tag = "class_attribute"

    for object in json_object.get('objects', []): 
        if object['type'] in ('class', 'struct'):
            fields_to_remove = []
            for field in object.get('fields', []):
                for annotation_key, annotation_value in field['annotations'].iteritems():
                    for attribute_name, attribute_value in annotation_value.iteritems():

                        if attribute_name.lower() == tag.lower():
                            fields_to_remove.append(field)

                            if annotation_key in object["annotations"]:
                                raise ValueError(
                                    "Warning - Duplicate class_attribute tag: {} "
                                    "in object {}".format(annotation_key, object['qualified_name']))
                                object["annotations"][annotation_key].update(annotation_value)
                            else:
                                object["annotations"][annotation_key] = annotation_value

            if remove_virtual_fields and len(fields_to_remove) > 0:
                for remove_field in fields_to_remove:
                    if remove_field in object.get('fields', []):
                        object["fields"].remove(remove_field)

def expand_annotations(source_dictionary):
    """Takes a partially extracted JSON tree generated by C++ and parses
    the annotations fields, expanding them into python dictionary
    trees.
    @param source_dictionary - The dictionary containing the annotation
                               fields to expand.
    """
    def expand_and_store_annotation(dest, key, tag, value):
        # extract any template params if they exist
        match = TEMPLATE_TAG_PATTERN.match(tag)
        template_params = None
        if match:
            tag = match.group('tag')
            template_params = match.group('template_params')
        if template_params: # if there are template params, nest them
            value = { u'params': value, u'template_params': re.split('[\s,]+', template_params) }
        store_annotation(dest, tag, value)

    annotations = {}
    for annotation_key, annotation_value in source_dictionary['annotations'].iteritems():
        for attribute_name, attribute_value in annotation_value.iteritems():
            # The tree returned might be collapsible into a list
            # or a string. Check and perform the appropriate adjustment.
            result = build_tree_from_string(attribute_value)
            if is_simple_string(result):
                expand_and_store_annotation(annotations, annotation_key, attribute_name, convert_key_to_string(result))
            elif is_list(result):
                expand_and_store_annotation(annotations, annotation_key, attribute_name, convert_keys_to_list(result))
            else:
                if result is None: # tags with no arguments default to a true value
                    result = "true"
                expand_and_store_annotation(annotations, annotation_key, attribute_name, result)
    # update annotations with converted hierarchy
    source_dictionary[u'annotations'] = annotations


def build_tree_from_string(formatted_data):
    """ Recursively builds a dictionary tree out of a string using the
    following rules:
    Parameter lists: Token(Arguments...)  become Dictionary entries:
                     Token:Arguments...
    Any parameter list that is a string or list of strings gets saved as
    such.
    This will always return a dictionary, even if given a simple string
    or a list of strings. The is_simple_string and is_list detection
    functions and convert_key_to_string and convert_keys_to_list functions
    can be used to process the results further if necessary.
    @param formatted_data - A string of data formatted by the
                            AZCodeGenerator C++ Parser
    @return A dictionary containing all the data parsed out of the
            incoming string.
    """
    # must maintain key order, in case we convert the keys to a list
    result = OrderedDict() 

    # The AZCodeGen system can produce empty parameter lists
    # early detection will prevent python crashes
    if formatted_data is None:
        return None

    # This seems to happen when someone adds multiple of the same tag
    if not isinstance(formatted_data, str) and not isinstance(formatted_data, unicode):
        raise TypeError('Expecting string value, got {} ({})\nDid you add multiple of the same tag to an annotation?'.format(type(formatted_data), formatted_data))

    # remove whitespace
    formatted_data.strip()
    # strip surrounding {}s used in initializer_list<>s
    if formatted_data and formatted_data[0] == '{':
        formatted_data = re.sub(r'^\{(.+)\}$', r'\1', formatted_data)

    # The AZCodeGen system can produce empty strings as parameter lists
    # Early detection will speed up execution
    if not formatted_data:
        return None

    # AZCodeGen allows spaces, commas, and tabs to be used as major
    # delimiters
    # in lists
    delimiters = " ,\t"

    # terminal case (Simple Token)
    if is_simple_token(formatted_data, delimiters):
        result[formatted_data] = None
    # recursive case (New Parameter)
    elif is_token_and_params(formatted_data, delimiters):
        tag = extract_tag(formatted_data, delimiters)
        params = build_tree_from_string(extract_parameters(formatted_data))
        template_params = extract_template_params(formatted_data)
        # Determine if we can flatten the parameter list into a string
        # or a list of strings
        if is_simple_string(params):
            params = convert_key_to_string(params)
        elif is_list(params):
            params = convert_keys_to_list(params)
        
        if template_params:
            result[tag] = {
                u'params': params ,
                u'template_params': template_params,
            }
        else:
            result[tag] = params
        
    # There are multiple tokens At This Level.
    # Separate and parse them individually.
    else:
        args = split_on_major_delimiter(formatted_data, delimiters)
        trees = []
        for arg in args:
            tree = build_tree_from_string(arg)
            trees.append(tree)

        # pre-format the result dict, if there are multiple values at a key
        # ensure that the result value will be a list
        for tree in trees:
            for tag, value in tree.iteritems():
                if tag in result and not isinstance(result[tag], list):
                    result[tag] = []
                elif tag not in result:
                    result[tag] = None

        # coalesce value trees into the result
        for tree in trees:
            for tag, value in tree.iteritems():
                if isinstance(result[tag], list):
                    result[tag].append(value)
                else:
                    result[tag] = value

    return result


def string_detector( string_data ):
    """Generator that runs through a string and tracks weather or not it's
    inside inside a set of double-quotes at any given point in time.
    Full support for escaped quotes is supported.
    Example:
    string_detector("This \"is in a \\\"String\\\"\" right?")
                           ^^^^^^^^^^^^^^^^^^^^^^^^
    will return true only when iterating over the characters flagged
    by the ^s.
    @param string_data - A string to iterate over
    @return A bool, false while the string being iterated over is
            outside of matching quotes, true while inside.
    """
    escaped = False
    in_string = False

    for c in string_data:
        # Handle Strings
        if not escaped and c == '\"':
            if in_string:
                in_string = False
            else:
                in_string = True

        # Handle Escape Sequences
        if c == '\\' and escaped == False:
            escaped = True
        else:
            escaped = False

        yield in_string


def is_simple_token(string_data, delimiters):
    """Returns true if the given string doesn't contain any delimiters or
    parenthesis that aren't contained inside a quotation. This is the
    terminal case when parsing a JSON string.
    @param string_data - The string we're going to check
    @param delimiters - A string containing all the delimiters that could
                        separate valid tokens.
    @return True if string_data is a simple token, False otherwise
    """
    valid = True
    index = 0

    string_checker = string_detector(string_data)

    for c in string_data:
        in_string = next(string_checker)

        # Handle Parenthesis
        if not in_string and c == '(':
            # Any opening parenthesis outside of strings invalid tokens
            valid = False

        # Find Separators
        if not in_string:
            if delimiters.find(c) != -1:
                # Major Delimiters Invalidate Tokens
                valid = False

        # increment our index
        index += 1

        if not valid:
            break

    return valid


def is_token_and_params(string_data, delimiters):
    """return True if the string is a single token with data in parenthesis
    For Example:
      "Token(\"Arguments\")"           will return True
      "Token(\"Arguments\"),Token"     will return False
    Used to detect tokens with arguments
    make sure to trim excess whitespace from the beginning and end
    of the source string
    @param string_data - The string to check for a parametrized token
    @param delimiters - A string containing all the delimiters that could
                        separate valid tokens.
    @return True if string_data contains a parametrized token, false
            otherwise
    """
    parenthesis_depth = 0
    parenthesis_found = False
    zero_count = 0
    last_close = 0
    valid = True
    index = 0

    string_checker = string_detector(string_data)

    for c in string_data:
        in_string = next(string_checker)

        # Handle Parenthesis
        if not in_string and c == '(':
            # a parametrized token requires a tag before parenthesis
            if index == 0:
                valid = False
                break
            parenthesis_depth += 1
            parenthesis_found = True

        # Handle closing parenthesis
        if not in_string and c == ')':
            parenthesis_depth -= 1
            if parenthesis_depth == 0:
                zero_count += 1
                last_close = index + 1

        # Find Delimiters
        if not in_string and parenthesis_depth == 0:
            # Major Delimiters Invalidate all Tokens
            if delimiters.find(c) != -1:
                valid = False
                break

        # increment our index
        index += 1

    return valid and parenthesis_found and zero_count == 1 and last_close == index


def extract_tag(string_data, delimiters):
    """Extract and return a tag associate with a parameter
    list in a string.
    For example:
    "Token(Parameter)" would return "Token"
    Note: The incoming string must be validated by is_parametrized_token
    @param string_data - A string containing a parametrized token to
                         extract the tag from
    @param delimiters - A string containing all the delimiters that
                        could separate valid tokens.
    @return A string containing the tag associate with the parametrized
            token
    """
    found = False
    index = 0

    string_checker = string_detector(string_data)

    for c in string_data:
        in_string = next(string_checker)

        # Handle Parenthesis
        if not in_string and (c == '(' or c == '<'):
            found = True
            break

        # Major delimiters can't be found before the parameter
        # tag is discovered.
        if not in_string:
            if delimiters.find(c) != -1:
                break

        # increment our index
        index += 1

    if found:
        return string_data[:index]
    else:
        return "null"


def extract_parameters(string_data):
    """Extract and return the parameters contained in parenthesis
    of a parametrized JSON entry in a string.
    For example:
    "Token(Parameter)" would return "Parameter"
    Note: The incoming string must be validated by is_parametrized_token
    @param string_data - The incoming string to extract parameters from
    @return A string containing the parameter string of the input token
    """
    parenthesis_depth = 0
    open_index = 0
    close_index = 0
    index = 0

    string_checker = string_detector(string_data)

    for c in string_data:
        in_string = next(string_checker)

        # Handle Parenthesis
        if not in_string and c == '(':
            parenthesis_depth += 1
            if parenthesis_depth == 1:
                open_index = index + 1
        if not in_string and c == ')':
            parenthesis_depth -= 1
            if parenthesis_depth == 0:
                close_index = index

        # increment our index
        index += 1

    if open_index > close_index:
        return "null"
    else:
        return string_data[open_index:close_index]

def extract_template_params(string_data):
    """Extract and return the parameters contained in parenthesis
    of a parametrized JSON entry in a string.
    For example:
    "Token<T<U,V>, K>(Parameter)" would return "T<U,V>, K"
    Note: The incoming string must be validated by is_templated_token
    @param string_data - The incoming string to extract parameters from
    @return A string containing the parameter string of the input token
    """

    template_depth = 0
    open_index = 0
    close_index = 0
    index = 0

    string_checker = string_detector(string_data)

    for c in string_data:
        in_string = next(string_checker)

        # handle template params
        if not in_string and c == '<':
            template_depth += 1
            if template_depth == 1:
                open_index = index + 1
        if not in_string and c == '<':
            template_depth -= 1
            if template_depth == 0:
                close_index = index

        # increment our index
        index += 1

    if open_index > close_index:
        return "null"
    else:
        return string_data[open_index:close_index]


def split_on_major_delimiter(string_data, delimiters):
    """Split a string using the given separator but only
    when the separator isn't contained in a string or
    a set of valid parenthesis
    @param string_data - the string of delimited arguments
    @param delimiters - The delimiters to split the string on
    @return a list of strings containing the individual elements
            of the input string.
    """
    parenthesis_depth = 0
    template_depth = 0
    brace_depth = 0
    start = 0
    index = 0
    result = []

    string_checker = string_detector( string_data )

    for c in string_data:

        in_string = next(string_checker)

        if not in_string:
            # Handle Parenthesis
            if c == '(':
                parenthesis_depth += 1
            elif c == ')':
                parenthesis_depth -= 1
            elif c == '<':
                template_depth += 1
            elif c == '>':
                template_depth -= 1
            elif c == '{':
                brace_depth += 1
            elif c == '}':
                brace_depth -= 1

            # Find Separators
            if (parenthesis_depth + template_depth + brace_depth) == 0:
                if delimiters.find(c) != -1:
                    # Allow consecutive major delimiters
                    if index != start:
                        # Save the token
                        token = string_data[start:index]
                        result.append(string_data[start:index])
                    # Set the start of the next token to the next character
                    start = index + 1

        # increment our index
        index += 1

    # handle the last token
    if index != start:
        result.append(string_data[start:])

    return result


############################################################
# Helper Functions
############################################################

def is_simple_string(test_dictionary):
    """These take dictionaries and determine if they can be converted to
    various types simpler strings or lists of strings or perform
    the actual conversions


    Detect if a dictionary can be converted to a string
    @param test_dictionary - A dictionary to test
    @return False if the incoming dictionary is empty, has multiple
            key/value pairs, or has any values at all that
            aren't the type None. True if the dictionary has
            one key/value pair and the value is of type None.
    """
    assert not test_dictionary or isinstance(test_dictionary, OrderedDict)
    return test_dictionary and len(test_dictionary) == 1 and test_dictionary.values()[0] is None


def is_list(test_dictionary):
    """Detect if a dictionary can be converted into
    a list of strings
    @param test_dictionary - The dictionary to test
    @return False if the incoming dictionary is empty, has only a single
            key/value pair, or has any values at all that aren't
            the None type. True if the dictionary has multiple
            key/value pairs and the values are all of type None.
    """
    assert not test_dictionary or isinstance(test_dictionary, OrderedDict)
    if test_dictionary and len(test_dictionary) > 1:
        for value in test_dictionary.values():
            if value is not None:
                return False
        return True
    return False


def convert_keys_to_list(source_dictionary):
    """Convert all the keys in a dictionary into a list of strings
    make sure to verify that the incoming dictionary can
    be converted to a list using the above is_list() function
    @param source_dictionary - A dictionary to convert
    @return A list of strings derived from the keys of the
            incoming dictionary.
    """
    result = []
    for key in source_dictionary.keys():
        result.append(str(key))
    return result


def convert_key_to_string(source_dictionary):
    """Convert the first key found in a Dictionary to a string
    make sure to verify that the incoming dictionary
    can be converted to a string using the
    is_simple_string() above
    @param source_dictionary - The dictionary containing the string
    @return A string derived from first key found in the
            incoming dictionary
    """
    return source_dictionary.keys()[0]

###############################################################################
# Legacy CodeGen conversion
###############################################################################
ATTRIBUTES_MAP = {
    "uiId": "UIHandler",
}

def convert_annotations(object):
    """ fixup field annotations from old format -> new format
    @param object JSON object representing a class from the AST
    """
    for field in object.get('fields', []):
        annotations = field['annotations']
        field_annotations = {'AzProperty': {}}
        for annotation_key, annotation_value in annotations.iteritems():
            # squash the old annotations -> AzProperty
            if annotation_key in ('AzEdit', 'AzEditVirtual', 'AzSerialize'):
                # promote nested attributes to top level
                if 'attribute' in annotation_value:
                    attributes = annotation_value['attribute']
                    for attr, val in attributes.iteritems():
                        new_attr = 'AzProperty::Attribute::' + attr[0:1].upper() + attr[1:]
                        annotation_value[new_attr] = val
                # convert to AzProperty attributes
                for tag_name, tag_value in annotation_value.iteritems():
                    new_tag_name = tag_name
                    if tag_name in ATTRIBUTES_MAP:
                            new_tag_name = ATTRIBUTES_MAP[tag_name]
                    new_tag_name = 'AzProperty::' + new_tag_name[0:1].upper() + new_tag_name[1:]
                    field_annotations['AzProperty'][new_tag_name] = tag_value
            else:
                field_annotations[annotation_key].update(annotation_value)
            field['annotations'] = field_annotations
