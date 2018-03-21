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

from cgf_utils.version_utils import Version

INTERFACE_IMPLEMENTATION_OBJECT_NAME = 'x-cloud-gem-framework-interface-implementation'
INTERFACE_METADATA_OBJECT_NAME = 'x-cloud-gem-framework-interface-metadata'
INTERFACE_PARAMETER_OBJECT_NAME = 'x-cloud-gem-framework-interface-parameter'

def parse_interface_id(interface_id):
    '''Returns a typle with three values: resource-group-name, interface-name, and interface-version'''
    try:
        parts = interface_id.split('_')
        if len(parts) != 5:
            raise RuntimeError()
        return parts[0], parts[1], Version('.'.join(parts[2:5]))
    except:
        raise RuntimeError('Could not parse interface id: {}. Expecting GemName_InterfaceName_Major_Minor_Revision.'.format(interface_id))
