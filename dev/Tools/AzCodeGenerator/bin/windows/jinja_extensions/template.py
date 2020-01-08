"""
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import jinja2
import re
import types
import uuid


def toUuid(fullClassName):
    """
    Convert strings to Uuid
    """
    return uuid.uuid5(uuid.NAMESPACE_URL, str(fullClassName))  # Use SHA-1


def asArray(parameter):
    # The input parameter may be Undefined by jinja.
    # If this happens, we return an empty, valid array.
    if isinstance(parameter, jinja2.runtime.Undefined):
        return []

    # If the parameter is not already a list, we return an array with it as the first element.
    if not isinstance(parameter, list):
        return [parameter]

    # The parameter is already a list, return it.
    return parameter


def asStringIdentifier(s):
    """
    Substitute for jinja filter combination: |replace("/", "_")|replace(":","_")|replace(".","_")
    """
    assert isinstance(s, basestring)
    return re.sub(r'[/:.]', '_', s)


def stripConstAndRef(s):
    assert isinstance(s, basestring)
    if s.startswith('const '):
        s = s[len('const '):]
    while s.endswith('&'):
        s = s[:-len('&')]
    return s.strip()


def capitalizeFirst(s):
    assert isinstance(s, basestring)
    return s[0].upper() + s[1:]


def openNamespacesOf(s):
    assert isinstance(s, basestring)
    return '\n'.join('namespace {} {{'.format(n)
                     for n in s.split('::')[:-1])


def closeNamespacesOf(s):
    assert isinstance(s, basestring)
    return '\n'.join('}} // namespace {}'.format(n)
                     for n in s.split('::')[:-1])


def stripNamespaces(s):
    assert isinstance(s, basestring)
    return s.split('::')[-1]


# ---------------------------------------------------
def registerExtensions(jinjaEnv):
    """
    Expose the function into the global function list in the template.
    """
    assert isinstance(jinjaEnv, jinja2.Environment)

    def reg(f):
        assert isinstance(f, types.FunctionType)
        jinjaEnv.globals[f.func_name] = f

    reg(toUuid)
    reg(asArray)
    reg(asStringIdentifier)
    reg(stripConstAndRef)
    reg(capitalizeFirst)
    reg(openNamespacesOf)
    reg(closeNamespacesOf)
    reg(stripNamespaces)
