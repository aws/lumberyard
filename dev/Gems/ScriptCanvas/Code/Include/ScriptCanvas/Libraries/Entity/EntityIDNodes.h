/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <ScriptCanvas/Core/NodeFunctionGeneric.h>

namespace ScriptCanvas
{
    namespace EntityIDNodes
    {
        using namespace Data;
        
        AZ_INLINE BooleanType IsValid(const EntityIDType& source)
        {
            return source.IsValid();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(IsValid, "Entity/Game Entity", "{0ED8A583-A397-4657-98B1-433673323F21}", "returns true if Source is valid, else false", "Source");
        
        AZ_INLINE StringType ToString(const EntityIDType& source)
        {
            return source.ToString();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ToString, "Entity/Game Entity", "{B094DCAE-15D5-42A3-8D8C-5BD68FE6E356}", "returns a string representation of Source", "Source");

        using Registrar = RegistrarGeneric
            < IsValidNode
            , ToStringNode
            > ;

    } // namespace EntityIDNodes
} // namespace ScriptCanvas

