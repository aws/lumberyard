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
from datetime import datetime
import os
from xml.etree.ElementTree import Element, ElementTree, SubElement
from aztest.errors import InvalidUseError


def create_xml_output_file(path_to_file, error_code, error_msg):
    """
    Produces a template xml file at path to file that contains the error code and error message as text.

    :param path_to_file: The full path to the file to be generated
    :param error_code: The error code returned by the test runner failure
    :param error_msg: The error message returned by the test runner failure
    :return: None
    """
    if not path_to_file:
        raise InvalidUseError("XML creation requires path to XML output file")

    try:
        error_code = int(error_code)
    except TypeError:
        raise InvalidUseError("The error code produced by the test runner should be an integer")

    if not isinstance(error_msg, str):
        raise InvalidUseError("The error message produced by the test runner should be of type basestring")

    current_time = datetime.now().strftime("%Y-%m-%dT%X")
    # Create the attributes required for the HTMLReporter
    testsuite_attributes = {"tests": "0", "time": "0.0",  "failures": "0", "disabled": "0", "errors": "1",
                            "timestamp": current_time}

    xml_root = Element("testsuites", testsuite_attributes)
    properties = SubElement(xml_root, "properties")
    error_code_property = SubElement(properties, "property", name="ErrorCode", value=str(error_code))
    error_msg_property = SubElement(properties, "property", name="ErrorMsg", value=error_msg)
    testsuite = SubElement(xml_root, "testsuite", attrib=testsuite_attributes, name=os.path.basename(path_to_file))
    testcase = SubElement(testsuite, "testcase", name="AzRunTests", time="0")
    error = SubElement(testcase, "error", message="Failed with error code {}: {}".format(error_code, error_msg))
    xml_tree = ElementTree(xml_root)
    with open(path_to_file, "w") as xml_file:
        xml_tree.write(xml_file, encoding="UTF-8", xml_declaration=True)
